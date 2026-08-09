import socket, struct

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('0.0.0.0', 5601))
print("Listening on port 5601...")

while True:
    data, addr = sock.recvfrom(4096)
    ts = struct.unpack_from('I', data, 0)[0]
    count = struct.unpack_from('B', data, 4)[0]
    print(f'ts={ts} count={count}')
    for i in range(count):
        off = 8 + i * 20
        id_, x1, y1, x2, y2 = struct.unpack_from('iffff', data, off)
        print(f'  id={id_} x1={x1:.3f} y1={y1:.3f} x2={x2:.3f} y2={y2:.3f}')
