#include <array>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "decoder_versions/decoder.hpp"
#include "output_components/AllMessageTypes.h"
#include "output_components/4.4 Tags.h"

namespace {
volatile std::sig_atomic_t g_stop_requested = 0;

void handle_signal(int) {
    g_stop_requested = 1;
}

void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [host] [port] [--log FILE]\n"
              << "  host        FIX feed host              (default 127.0.0.1)\n"
              << "  port        FIX feed port              (default 8080)\n"
              << "  --log FILE  write structured JSON-lines log to FILE\n"
              << "              (omit for pure measurement mode; stats go to stderr)\n";
}
} // namespace

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);

    std::string host = "127.0.0.1";
    unsigned long port = 8080;
    std::string log_path;
    std::vector<std::string> positional;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--log") {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 2;
            }
            log_path = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else {
            positional.push_back(arg);
        }
    }

    if (positional.size() > 2) {
        print_usage(argv[0]);
        return 2;
    }
    if (positional.size() >= 1) host = positional[0];
    if (positional.size() >= 2) {
        try {
            port = std::stoul(positional[1]);
        } catch (const std::exception&) {
            std::cerr << "FIX_parser: invalid port: " << positional[1] << '\n';
            return 2;
        }
    }
    if (port == 0 || port > 65535) {
        std::cerr << "FIX_parser: port out of range: " << port << '\n';
        return 2;
    }

    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to Create Socket :(" << '\n';
        return 1;
    }
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, host.c_str(), &server.sin_addr) <= 0) {
        std::cerr << "invalid address: " << host << '\n';
        close(sock);
        return 2;
    }

    if (connect(sock, reinterpret_cast<sockaddr*>(&server), sizeof(server)) < 0) {
        std::cerr << "Connect error\n";
        close(sock);
        return 1;
    }
    std::cout << "Connected successfully" << '\n';

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    const bool logging = !log_path.empty();
    std::ofstream log_file;
    if (logging) {
        log_file.open(log_path);
        if (!log_file) {
            std::cerr << "FIX_parser: cannot open log file: " << log_path << '\n';
            close(sock);
            return 2;
        }
    }

    std::array<char, 8192> buffer{};
    size_t offset = 0;
    size_t total_messages = 0;
    auto total_parse_time = std::chrono::nanoseconds::zero();
    std::vector<std::string> decoded;

    bool clean_close = false;
    while (!g_stop_requested) {
        pollfd pfd{sock, POLLIN, 0};
        const int rc = poll(&pfd, 1, 200);
        if (rc < 0) {
            if (g_stop_requested) break;
            std::cerr << "poll error\n";
            break;
        }
        if (rc == 0) continue; // timeout: re-check stop flag

        if (offset >= buffer.size()) {
            std::cerr << "Buffer exhausted without a complete message; aborting\n";
            break;
        }

        const ssize_t bytes_read = recv(sock, buffer.data() + offset, buffer.size() - offset, 0);
        if (bytes_read > 0) {
            const size_t total_bytes = offset + static_cast<size_t>(bytes_read);

            const auto start = std::chrono::high_resolution_clock::now();

            size_t parsed_messages = 0;
            const size_t consumed_bytes = logging
                ? decoder(buffer, total_bytes, tags44, fix44MsgTypes, &parsed_messages, &decoded)
                : decoder(buffer, total_bytes, tags44, fix44MsgTypes, &parsed_messages);

            const auto end = std::chrono::high_resolution_clock::now();
            total_parse_time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            total_messages += parsed_messages;

            if (logging) {
                for (const auto& line : decoded) log_file << line << '\n';
                decoded.clear();
            }

            const size_t leftover_bytes = total_bytes - consumed_bytes;
            if (leftover_bytes > 0 && consumed_bytes > 0) {
                std::memmove(buffer.data(), buffer.data() + consumed_bytes, leftover_bytes);
            }
            offset = leftover_bytes;
        } else if (bytes_read == 0) {
            std::cout << "Connection closed\n";
            clean_close = true;
            break;
        } else {
            std::cerr << "Receive failed\n";
            break;
        }
    }

    if (g_stop_requested && !clean_close) {
        std::cout << "Shutdown requested, closing connection\n";
    }
    close(sock);

    const double ns = static_cast<double>(total_parse_time.count());
    std::cerr << "Messages parsed:   " << total_messages << '\n'
              << "Total parse time:  " << total_parse_time.count() << " ns ("
              << ns / 1000000.0 << " ms)\n";
    if (total_messages > 0) {
        std::cerr << "Avg parse time:    " << ns / static_cast<double>(total_messages) << " ns/msg\n"
                  << "Throughput:        " << static_cast<double>(total_messages) / (ns / 1e9)
                  << " msg/s\n";
    }
    return 0;
}