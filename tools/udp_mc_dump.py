import socket
import struct
import sys
import time

"""
Join multicast 225.225.225.225:13134 and print packet arrivals.
Helps confirm Multicast_Status stream is present.
"""

MCAST_GRP = '225.225.225.225'
MCAST_PORT = 13134

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
try:
    sock.bind(('', MCAST_PORT))
except OSError as e:
    print(f"Bind failed: {e}")
    sys.exit(1)

mreq = struct.pack("4sl", socket.inet_aton(MCAST_GRP), socket.INADDR_ANY)
sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
print(f"Joined {MCAST_GRP}:{MCAST_PORT}. Waiting for packets...")

cnt = 0
try:
    while True:
        data, addr = sock.recvfrom(65535)
        cnt += 1
        print(time.strftime('%H:%M:%S'), f"#{cnt} from {addr[0]}:{addr[1]} len={len(data)}")
except KeyboardInterrupt:
    print("Stopped.")
