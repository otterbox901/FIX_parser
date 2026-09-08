import argparse
import socket

from scapy.all import rdpcap, TCP, Raw


def replay_pcap(pcap_file, host='127.0.0.1', port=8080):
    packets = rdpcap(pcap_file)

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((host, port))
    server.listen(1)

    print(f"Listening on {host}:{port}...")
    conn, addr = server.accept()
    print(f"Client connected from {addr}")

    for pkt in packets:
        if pkt.haslayer(TCP) and pkt.haslayer(Raw):
            payload = pkt[Raw].load
            conn.sendall(payload)

    print("Finished replaying PCAP.")
    conn.close()
    server.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Replay a PCAP's TCP payloads to a FIX parser client.")
    parser.add_argument("pcap", help="path to the PCAP file containing FIX TCP traffic")
    parser.add_argument("--host", default="127.0.0.1", help="bind host (default 127.0.0.1)")
    parser.add_argument("--port", type=int, default=8080, help="bind port (default 8080)")
    args = parser.parse_args()
    replay_pcap(args.pcap, args.host, args.port)