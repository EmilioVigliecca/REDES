import socket

def parseo_error(host, port, body):
    req = (
        "POST /RPC2 HTTP/1.1\r\n"
        f"Host: {host}\r\n"
        "User-Agent: Test\r\n"
        "Content-Type: text/xml\r\n"
        f"Content-Length: {len(body.encode())}\r\n"
        "\r\n"
        f"{body}"
    )
    s = socket.socket()
    s.connect((host, port))
    s.sendall(req.encode())
    data = s.recv(65536)
    print(data.decode(errors='ignore'))
    s.close()

# 1) cuerpo vacío parse error
parseo_error("127.0.0.1", 5000, "")

# 2) XML malformado (falta que cierre el <value>)
mal = "<?xml version='1.0'?><methodCall><methodName>eco</methodName><params><param><value><string>hola</string></param></params></methodCall>"
parseo_error("127.0.0.1", 5000, mal)

