import socket
import xml.etree.ElementTree as ET

class Client:

    def __init__(self, address, port):
        self.address = port
        self.port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect(address, port)

    def __getattr__(self, nombre_metodo):
        #Esta funcion se llama cuando se invoca un metodo que no esta definido: 
        
        def metodo_remoto(*args):
            #este metodo deberia ser capaz de:
            # - armar un XML
            parametros = "".join(
                f"<param><value><int>{a}</int></value></param>" for a in args
            )
             
            xml = f"""<?xml version="1.0"?>
            <methodCall>
              <methodName>{nombre_metodo}</methodName>
              <params>{parametros}</params>
            </methodCall>"""

            request = f"POST /RPC2 HTTP/1.1\r\nHost: {self.address}\r\nContent-Length: {len(xml)}\r\n\r\n{xml}"

            # - mandarlo al servidor
            self.socket.send(request.encode()) 
            
            # - recibir respuesta
            data , err = self.socket.recv(4096).decode()
            
            # - parsear XML y devolver resultadore
            respuesta = data.split("\r\n\r\n", 1)[1]
            
            return 

        return metodo_remoto

def connect(address, port):
    return Client(address, port)

#class Client:
#    def connect(adress, port):
#        return Client(adress, port)

class Server:
    def __init__(self, address, port):
        self.address = port
        self.port = port
        self.metodos = {}
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.bind(address, port)
        self.socket.listen(1)
        #capaz podriamos hacer que el server notifique


    def add_method(self, proc1):
        self.metodos.add(proc1) 

    def serve(self):
        while True:
            #INLCUIR THREADS PARA QUE PUEDA ATENDER A MAS DE UN CLIENTE A LA VEZ
            
            #aceptar conexiones:
            conn, addr = self.socket.accept()
            #recibir el xml
            data = conn.recv(4096).decode()
            #identificar que funcion pidio
            respuesta = ""
            conn.send(respuesta)
            conn.close()