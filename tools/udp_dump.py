import argparse
import socket
import sys
import time

"""
Simple UDP dumper to verify whether packets arrive on a given port.
Prints every packet with timestamp, source address and length.

Usage:
  python udp_dump.py --bind 0.0.0.0 --port 14134
"""

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--bind", default="0.0.0.0")
    p.add_argument("--port", type=int, required=True)
    args = p.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        sock.bind((args.bind, args.port))
    except OSError as e:
        print(f"Bind failed on {args.bind}:{args.port} -> {e}")
        sys.exit(1)

    print(f"Listening UDP on {args.bind}:{args.port} ... (Ctrl+C to stop)")
    cnt = 0
    try:
        while True:
            data, addr = sock.recvfrom(65535)
            cnt += 1
            print(time.strftime('%H:%M:%S'), f"#{cnt} from {addr[0]}:{addr[1]} len={len(data)}")
    except KeyboardInterrupt:
        print("Stopped.")

if __name__ == "__main__":
    main()
