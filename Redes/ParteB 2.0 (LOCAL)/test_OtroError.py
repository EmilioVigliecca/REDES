import socket

body = "<?xml version='1.0'?><methodCall><methodName>saludar</methodName><params></params></methodCall>"
req = (
    "POST /RPC2 HTTP/1.0\r\n"
    "Host: 127.0.0.1\r\n"
    "User-Agent: Test\r\n"
    "Content-Type: text/xml\r\n"
    f"Content-Length: {len(body.encode()) + 2000}\r\n"  # intencionalmente grande
    "\r\n"
    f"{body}"
)
s = socket.socket()
s.connect(("127.0.0.1", 5000))
s.sendall(req.encode()) # no envío más datos -> el servidor quedará esperando y luego dará ConnectionError
data = s.recv(65536)
print(data.decode(errors='ignore'))
s.close()