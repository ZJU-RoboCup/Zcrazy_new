import argparse
import socket
import sys

"""
Forward all incoming UDP packets from local port 14134 to a target IP (Android phone) on the same port.
Useful when robots send Robot_Status only to the PC, not to the phone.

Usage:
  python robot_status_forward.py --target 192.168.31.123 --listen 0.0.0.0 --port 14134
"""

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--target", required=True, help="Target IP (Android phone) to forward to")
    p.add_argument("--listen", default="0.0.0.0", help="Local bind address (default 0.0.0.0)")
    p.add_argument("--port", type=int, default=14134, help="UDP port to listen and forward (default 14134)")
    args = p.parse_args()

    sock_in = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock_in.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock_in.bind((args.listen, args.port))

    sock_out = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    print(f"Forwarding UDP {args.listen}:{args.port} -> {args.target}:{args.port}")
    count = 0
    try:
        while True:
            data, addr = sock_in.recvfrom(65535)
            count += 1
            if count % 50 == 1:
                print(f"recv {count} from {addr[0]}:{addr[1]} len={len(data)} -> {args.target}:{args.port}")
            # optionally filter: only forward from robots subnet
            sock_out.sendto(data, (args.target, args.port))
    except KeyboardInterrupt:
        print("\nStopped.")
        sys.exit(0)

if __name__ == "__main__":
    main()
