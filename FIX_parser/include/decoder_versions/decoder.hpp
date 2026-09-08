#ifndef FIX_PARSER_DECODER_HPP
#define FIX_PARSER_DECODER_HPP
#include <array>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// Parses FIX messages from a buffer, returning the number of consumed bytes
// (everything up to and including the last complete message).
//
// The two modes are split into fully separate loops so the high-frequency
// measurement path costs nothing extra:
//
//   * log_lines == nullptr  -> measurement loop. This is byte-for-byte the
//     original decoder logic (same per-field lookups, no allocation); the
//     only addition is an optional per-message counter.
//
//   * log_lines != nullptr  -> structured logging loop: appends one
//     JSON-lines record per complete message (msgtype, name, fields, raw).
inline size_t decoder(const std::array<char, 8192>& buffer, const size_t& total_bytes,
                      const std::unordered_map<int, std::string>& tags,
                      const std::unordered_map<std::string, std::string>& message,
                      size_t* messages_count = nullptr,
                      std::vector<std::string>* log_lines = nullptr)
{
    if (log_lines != nullptr)
    {
        // ---------- structured logging path (JSON-lines) ----------
        const char* ptr = buffer.data();
        const char* end = ptr + total_bytes;
        const char* msg_start = ptr;
        int tag = 0;
        const char* val_start = nullptr;

        std::string msg_type;
        std::string msg_type_name;
        std::string fields;
        bool field_started = false;

        auto json_escape = [](std::string_view s) {
            std::string out;
            out.reserve(s.size() + 8);
            for (unsigned char c : s) {
                switch (c) {
                    case '"': out += "\\\""; break;
                    case '\\': out += "\\\\"; break;
                    case 0x01: out += '|'; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default:
                        if (c < 0x20) {
                            static constexpr char hex[] = "0123456789abcdef";
                            out += "\\u00";
                            out += hex[(c >> 4) & 0x0F];
                            out += hex[c & 0x0F];
                        } else {
                            out += static_cast<char>(c);
                        }
                }
            }
            return out;
        };

        auto append_field = [&](int t, std::string_view v) {
            if (field_started) fields += ',';
            else field_started = true;

            fields += '"';
            if (auto it = tags.find(t); it != tags.end()) {
                fields += it->second;
            } else {
                fields += std::to_string(t);
            }
            fields += "\":\"";
            fields += json_escape(v);
            fields += '"';
        };

        while (ptr < end) {
            if (*ptr == 0x3D) [[unlikely]] {
                val_start = ptr + 1;

                // SIMD for next SOH
                if (auto next_soh = static_cast<const char*>(memchr(val_start, 0X01, end - val_start))) [[likely]]
                {
                    std::string_view value(val_start, next_soh - val_start);

                    if (tag == 35) [[unlikely]] {
                        msg_type.assign(value);
                        if (message.contains(msg_type)) [[likely]] {
                            msg_type_name = message.at(msg_type);
                        }
                    } else {
                        if (tag == 8) [[unlikely]] {} // BeginString: message begin
                        append_field(tag, value);
                        if (tag == 10) [[unlikely]] {
                            std::string line;
                            line.reserve(fields.size() + msg_type.size() + msg_type_name.size() + 128);
                            line += "{\"msgtype\":\"";
                            line += json_escape(msg_type);
                            line += "\",\"name\":\"";
                            line += json_escape(msg_type_name);
                            line += "\",\"fields\":{";
                            line += fields;
                            line += "},\"raw\":\"";
                            line += json_escape(std::string_view(msg_start, next_soh - msg_start));
                            line += "\"}";
                            log_lines->push_back(std::move(line));
                            fields.clear();
                            field_started = false;
                            msg_type.clear();
                            msg_type_name.clear();
                            if (messages_count) ++(*messages_count);
                            msg_start = next_soh + 1;
                        }
                    }
                    ptr = next_soh;
                    tag = 0;
                }
                // no SOH found, wait for more data
                else break;
            } else [[likely]] {
                tag = tag * 10 + (*ptr - '0');
            }
            ++ptr;
        }
        return static_cast<size_t>(msg_start - buffer.data());
    }

    // ---------- measurement path (original decoder logic, no allocations) ----------
    const char* ptr = buffer.data();
    const char* end = ptr + total_bytes;
    const char* msg_start = ptr;
    int tag = 0;
    const char* val_start = nullptr;

    while (ptr < end) {
        if (*ptr == 0x3D) [[unlikely]] {
            val_start = ptr + 1;

            // SIMD for next SOH
            if (auto next_soh = static_cast<const char*>(memchr(val_start, 0X01, end - val_start))) [[likely]]
            {
                std::string_view value(val_start, next_soh - val_start);

                if (tag == 35) [[unlikely]] {
                    if (std::string str_val(value); message.contains(str_val)) [[likely]] {
                    }
                } else {
                    if (tag == 8) [[unlikely]] {} // BeginString: message begin
                    if (tags.contains(tag)) [[likely]] {} // known tag lookup
                    if (tag == 10) [[unlikely]] {
                        if (messages_count) ++(*messages_count);
                        msg_start = next_soh + 1;
                    }
                }
                ptr = next_soh;
                tag = 0;
            }
            // no SOH found, wait for more data
            else break;
        } else [[likely]] {
            tag = tag * 10 + (*ptr - '0');
        }
        ++ptr;
    }
    return static_cast<size_t>(msg_start - buffer.data());
}

#endif //FIX_PARSER_DECODER_HPP