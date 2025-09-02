import socket

class Client:
    def __init__ (self, address, port):
        self.address = address
        self.port = port
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket.connect((address, port))
    
    
    def __getattr__(self, nombre_metodo):

        def metodo_remoto(*args):
            #Aca se armaria el xml?
            xml = f"""<?xml version="1.0"?>
            <methodCall>
            ...
            """
            request = f"POST ...{xml}"
            #enviaria el request?
            self.client_socket.send(request.encode())
            #esperaria respuesta?
            response = self.socket.recv(4096).decode()
            #parsearia XML y devolveria resultado?
            return response
        
        
        
        return metodo_remoto 
    #Esto lo que hace es que si se hace conn = connect(address,port) y luego conn.suma(a,b), como conn no tiene suma, llama a metodo_remoto
 

    def __call__(self, *args, **kwds):
        pass

def connect(address, port):
    return Client(address, port)    
    
class Server:
    def __init__ (self, address, port):
        self.address = address
        self.port = port
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.connect((address, port))
        
    #def add_method (proc1):
        #append-(self, *args, **kwargs):
          #  return super().(*args, **kwargs)
        