import socket

HOST = "127.0.0.1"  #host local (no se puede conectar otra máquina de la red)
PORT = 5000

#Defino una función 
def start_server():
    #creo socket ipv4 (AF_INET) TCP (SOCK_STREAM)
    #with ... as s: significa que al terminar, el socket se cierra solo
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        #Conecto el socket a la direccion HOST y puerto PORT
        s.bind((HOST, PORT))
        #Encargo al SO para que me ponga en modo espera para aceptar conexiones(tipo MailBoxes)
        s.listen(1) #el 1 indica la cantidad de conexiones que puede tener en cola
        print(f"Servidor escuchando en {HOST}:{PORT}...") #f"..." es una f-string: permite meter variables dentro del texto
        #espero a que alguien se conecte:
        conn, addr = s.accept() #cuando alguien se conecta, se crea un nuevo socket en conn y addr toma el valor de la direccion del cliente
        with conn:
            print("Conexión establecida con:", addr)
            #recibo datos del cliente (hasta 4096 bytes)
            data = conn.recv(4096).decode() #decode() convierte de bytes a texto
            print("Mensaje recibido:\n", data)

            # Respuesta HTTP simple (sin XML todavía)
            response = (
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 12\r\n"
                "\r\n"
                "Hello RPC!\n"
            )
            #envío la respuesta codificada
            conn.send(response.encode())
#esta linea significa que si ejecuto el archivo directamente, se llama a la función que creé
if __name__ == "__main__":
    start_server()