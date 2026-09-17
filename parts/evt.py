import socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_addr = ("127.0.0.1", 60441)
while True:
  sock.sendto(b"SUB\n", server_addr)
  data, _ = sock.recvfrom(4096)
  print(data.decode("ascii", errors="replace"))
