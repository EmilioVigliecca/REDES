import threading
import socket
import xml.etree.ElementTree as ET

class Client:

    def __init__(self, address, port):
        self.server_address = address
        self.server_port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((address, port)) #Es tupla no sacar segundo parentesis

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

            request = f"POST /RPC2 HTTP/1.1\r\nHost: {self.server_address}\r\nContent-Length: {len(xml)}\r\n\r\n{xml}"

            # - mandarlo al servidor
            self.socket.sendall(request.encode()) 
            
            # - recibir respuesta
            data = self.socket.recv(4096)
            # El ,err no va me parece
            try:
                respuesta_xml = data.decode()
                root = ET.fromstring(respuesta_xml)
                result = root.find('result').text
                return result
            except (ET.ParseError, AttributeError, IndexError) as e:
                print(f"Error al parsear la respuesta: {e}")
                return None
            
        return metodo_remoto

def connect(address, port):
    return Client(address, port)


class Server:
    def __init__(self, address, port):
        self.address = address
        self.port = port
        self.metodos = {}
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.bind((address, port)) #recibe una tupla (address, port)
        self.socket.listen(5)
        print(f"Servidor escuchando en {self.address}:{self.port}")
        #capaz podriamos hacer que el server notifique
        #Y que pueda hacer threads, así puede atender a más de uno a la vez

    def add_method(self, proc1):
        self.metodos[proc1.__name__] = proc1

    #Funcion que se ejecuta por cada thread:
    def atender_cliente(self, conn, address):
        try:
            #aceptar conexiones:
            # Recibir datos completos del cliente.
            data = conn.recv(4096).decode()
            
            # Separar las cabeceras del cuerpo del mensaje HTTP.
            # La parte del cuerpo contiene el XML.
            header, xml_body = data.split('\r\n\r\n', 1)
            
            # Procesar el XML del cuerpo. (<methodCall en mensaje del cliente>)
            root = ET.fromstring(xml_body)
            
            # Extraer el nombre del método.
            method_name = root.find('methodName').text
            
            # Extraer los parámetros navegando por la estructura anidada.
            params_node = root.find('params')
            params = []
            if params_node is not None:
                for param_node in params_node.findall('param'):
                    # Encuentra el valor dentro de <value> e <int>
                    value_node = param_node.find('value/int')
                    if value_node is not None:
                        params.append(value_node.text)
            
            #identificar que funcion pidio:
            if method_name in self.metodos:

                #agregar excepcion parametros incorrecos
                
                result = self.metodos[method_name](*params)
                # Correcto: Envolver la respuesta en XML
                respuesta_xml = f"<response><result>{result}</result></response>"
                conn.sendall(respuesta_xml.encode())
            else:
                # Correcto: Envolver el error en XML
                respuesta_xml = f"<response><error>Metodo '{method_name}' no encontrado.</error></response>"
                conn.sendall(respuesta_xml.encode())

        except (ET.ParseError, IndexError, AttributeError) as e:
            # Manejar errores de parseo o de índice (ej. si no hay cuerpo HTTP)
            respuesta_xml = f"<response><error>XML o mensaje invalido: {e}</error></response>"
            conn.sendall(respuesta_xml.encode())
        except Exception as e:
            # Manejar cualquier otro error inesperado.
            respuesta_xml = f"<response><error>Error del servidor: {e}</error></response>"
            conn.sendall(respuesta_xml.encode())
        finally:
            conn.close()

    def serve(self):
        while True:
            #INLCUIR THREADS PARA QUE PUEDA ATENDER A MAS DE UN CLIENTE A LA VEZ
            conn, addr = self.socket.accept()
            print(f"Conexión aceptada de {addr}")

            client_thread = threading.Thread(target=self.atender_cliente, args=(conn, addr))
            
            # Inicia el hilo. Esto permite que el servidor vuelva a la línea `accept()`.
            client_thread.start()
