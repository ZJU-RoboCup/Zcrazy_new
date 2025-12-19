import socket,struct,time
from datetime import datetime
MCAST_GRP='225.225.225.225'
PORT=14134
MAX_PKTS=10

def read_varint(buf,i):
    out=0;shift=0
    while i < len(buf):
        b=buf[i];i+=1
        out |= (b & 0x7F) << shift
        if not (b & 0x80):
            return out,i
        shift +=7
        if shift>63: return None,i
    return None,i

def parse_tags(data):
    tags=[]
    i=0
    while i < len(data):
        tag=data[i];i+=1
        field=tag>>3;wire=tag & 0x07
        if field==0: break
        tags.append(field)
        if wire==0: # varint
            _,i=read_varint(data,i)
        elif wire==1: # 64-bit
            i+=8
        elif wire==2: # length-delimited
            ln,_i=read_varint(data,i);i=_i+ln
        elif wire==5: # 32-bit
            i+=4
        else:
            break
        if len(tags)>50: break
    return tags

def setup_multicast():
    msock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM,socket.IPPROTO_UDP)
    msock.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
    try:
        msock.bind(('',PORT))
    except OSError as e:
        print('Multicast bind fail:',e);return None
    mreq=struct.pack('4sl',socket.inet_aton(MCAST_GRP),socket.INADDR_ANY)
    try:
        msock.setsockopt(socket.IPPROTO_IP,socket.IP_ADD_MEMBERSHIP,mreq)
    except OSError as e:
        print('Join multicast fail:',e); return None
    msock.settimeout(0.7)
    return msock

def setup_unicast():
    usock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    usock.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
    try:
        usock.bind(('0.0.0.0',PORT))
    except OSError as e:
        print('Unicast bind fail:',e);return None
    usock.settimeout(0.7)
    return usock

def main():
    usock=setup_unicast()
    msock=setup_multicast()
    print('Sniffing Robot_Status on port',PORT,'(unicast + multicast) ...')
    count=0;start=time.time()
    while count < MAX_PKTS and time.time()-start < 15:
        for s,stype in ((usock,'UNI'),(msock,'MC')):
            if s is None: continue
            try:
                data,addr=s.recvfrom(65535)
            except socket.timeout:
                continue
            count+=1
            tags=parse_tags(data)
            print(f"{datetime.now().strftime('%H:%M:%S')} [{stype}] from {addr[0]} len={len(data)} tags={tags[:20]}")
        time.sleep(0.05)
    print('Done. Captured',count,'packets.')

if __name__=='__main__':
    main()
