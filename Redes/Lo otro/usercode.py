import socket

HOST = "127.0.0.1"
PORT = 5000

def send_request():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))

        # Request HTTP básico (POST con body)
        body = "<test>hola servidor</test>"
        request = (
            f"POST /RPC2 HTTP/1.1\r\n"
            f"Host: {HOST}\r\n"
            f"Content-Type: text/xml\r\n"
            f"Content-Length: {len(body)}\r\n"
            f"\r\n"
            f"{body}"
        )
        s.sendall(request.encode())

        response = s.recv(4096).decode()
        print("Respuesta del servidor:\n", response)

if __name__ == "__main__":
    send_request()