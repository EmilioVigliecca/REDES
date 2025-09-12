import threading
import socket
import xml.etree.ElementTree as ET
from email.utils import formatdate

class Client:
    def __init__(self, address, port):
        # Guardamos solo dirección y puerto, NO abrimos socket persistente
        self.server_address = address
        self.server_port = port

    def _get_xml_value(self, arg):
        if isinstance(arg, int):
            return f"<value><int>{arg}</int></value>"
        elif isinstance(arg, str):
            return f"<value><string>{arg}</string></value>"
        else:
            raise TypeError(f"Tipo de argumento no soportado: {type(arg)}")

    def _recibir_http(self, sock):
        data = b""
        while b"\r\n\r\n" not in data:
            chunk = sock.recv(16384)
            if not chunk:
                raise ConnectionError("Conexión cerrada antes de recibir headers")
            data += chunk

        header, resto = data.split(b"\r\n\r\n", 1)

        headers = header.decode().split("\r\n")
        content_length = 0
        for h in headers:
            if h.lower().startswith("content-length:"):
                content_length = int(h.split(":", 1)[1].strip())

        cuerpo = resto
        while len(cuerpo) < content_length:
            chunk = sock.recv(16384)
            if not chunk:
                raise ConnectionError("Conexión cerrada antes de recibir cuerpo completo")
            cuerpo += chunk

        return header.decode(), cuerpo.decode()

    def __getattr__(self, nombre_metodo):
        def metodo_remoto(*args):
            # Construir XML del request
            parametros = "\n".join(
                f"    <param>\n        {self._get_xml_value(a)}\n    </param>" for a in args
            )
            xml = f"""<?xml version="1.0"?>
        <methodCall>
            <methodName>{nombre_metodo}</methodName>
            <params>
        {parametros}
            </params>
        </methodCall>"""

            request = (
                f"POST /RPC2 HTTP/1.0\r\n"
                f"User-Agent: Cliente-XMLRPC\r\n"
                f"Host: {self.server_address}\r\n"
                f"Content-Type: text/xml\r\n"
                f"Content-Length: {len(xml.encode())}\r\n\r\n"
                f"{xml}"
            )

            # Abrir socket por cada request
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                sock.settimeout(60)
                sock.connect((self.server_address, self.server_port))
                sock.sendall(request.encode())
                header, xml_body = self._recibir_http(sock)

            # Procesar respuesta
            try:
                root = ET.fromstring(xml_body)
            except ET.ParseError as e:
                return f"Error al parsear XML: {e}"

            if root.tag == "methodResponse":
                param_node = root.find("params/param/value")
                if param_node is not None:
                    return param_node[0].text

                fault_string = root.find(
                    "fault/value/struct/member[name='faultString']/value/string"
                )
                return f"Error: {fault_string.text if fault_string is not None else 'desconocido'}"
            else:
                return "Respuesta inválida"
        return metodo_remoto

def connect(address, port):
    return Client(address, port)
    
class Server:
    def __init__(self, address, port):
        # Inicializa el servidor, enlaza y escucha en el puerto especificado.
        self.address = address
        self.port = port
        self.metodos = {}
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.bind((address, port)) #recibe una tupla (address, port)
        self.socket.listen(5)

        print(f"Servidor escuchando en {self.address}:{self.port}")

    def add_method(self, proc1):
            # Registra un metodo para ser llamado remotamente
            self.metodos[proc1.__name__] = proc1

    
    def construir_respuesta(self, xml):
        # Construye la respuesta HTTP completa
        content_length = len(xml.encode())
        headers = (
            "HTTP/1.1 200 OK\r\n"
            "Connection: close\r\n"
            f"Content-Length: {content_length}\r\n"
            "Content-Type: text/xml\r\n"
            f"Date: {formatdate(timeval=None, localtime=False, usegmt=True)}\r\n"
            "Server: XMLRPC-Python/1.0\r\n"
            "\r\n"
        )
        return headers + xml
    
    def recibir_http(self, conn):
        # Leer hasta encontrar el final de los headers (hasta \r\n\r\n)
        data = b""
        while b"\r\n\r\n" not in data:
            chunk = conn.recv(16384)
            if not chunk:
                raise ConnectionError("Conexión cerrada antes de recibir headers")
            data += chunk

        #Separo headers del resto
        header, resto = data.split(b"\r\n\r\n", 1)

        # Busco Content-Length
        headers = header.decode().split("\r\n")
        content_length = 0
        for h in headers:
            if h.lower().startswith("content-length:"):
                content_length = int(h.split(":", 1)[1].strip())

        # Sigo leyendo hasta completar el cuerpo entero
        cuerpo = resto
        
        #debug:
        #print(f"Esperando {content_length} bytes, ya tengo {len(resto)}")
        
        while len(cuerpo) < content_length:
            chunk = conn.recv(16384)
            if not chunk:
                raise ConnectionError("Conexión cerrada antes de recibir el cuerpo completo")
            cuerpo += chunk

            #debug:
            #print(f"Recibidos {len(cuerpo)}/{content_length}")
            
        return header.decode(), cuerpo.decode()

    def atender_cliente(self, conn, addr):
        print(f"Atendiendo a: {addr}")
        try:
            header, xml_body = self.recibir_http(conn)
            try:
                root = ET.fromstring(xml_body)
            except ET.ParseError:
                respuesta_xml = mensaje_fault(1, "Error de parseo XML")
                respuesta_http = self.construir_respuesta(respuesta_xml)
                conn.sendall(respuesta_http.encode())
                return

            method_name = root.find('methodName').text
            params = []
            params_node = root.find('params')
            if params_node is not None:
                for param_node in params_node.findall('param'):
                    value_node = param_node.find('value')
                    if value_node is not None:
                        int_node = value_node.find('int')
                        string_node = value_node.find('string')
                        if int_node is not None:
                            params.append(int(int_node.text))
                        elif string_node is not None:
                            params.append(string_node.text)
                        else:
                            respuesta_xml = mensaje_fault(3, "Error en parámetros")
                            respuesta_http = self.construir_respuesta(respuesta_xml)
                            conn.sendall(respuesta_http.encode())
                            return

            if method_name not in self.metodos:
                respuesta_xml = mensaje_fault(2, f"No existe el método invocado: '{method_name}'")
            else:
                try:
                    result = self.metodos[method_name](*params)
                    if isinstance(result, int):
                        respuesta_xml = f"""<?xml version="1.0"?>
<methodResponse>
<params>
    <param>
        <value><int>{result}</int></value>
    </param>
</params>
</methodResponse>"""
                    else:
                        respuesta_xml = f"""<?xml version="1.0"?>
<methodResponse>
<params>
    <param>
        <value><string>{result}</string></value>
    </param>
</params>
</methodResponse>"""
                except TypeError as e:
                    respuesta_xml = mensaje_fault(3, f"Error en parámetros: {e}")
                except Exception as e:
                    respuesta_xml = mensaje_fault(4, f"Error interno: {e}")

            respuesta_http = self.construir_respuesta(respuesta_xml)
            conn.sendall(respuesta_http.encode())   # envío la respuesta
        finally:
            conn.close()   # cierro la conexión después de cada request
             
            print(f"Conexión terminada con {addr}")


    def serve(self):
        # Bucle para aceptar nuevas conexiones de clientes
        while True:
            conn, addr = self.socket.accept()
            print(f"Conexión aceptada de {addr}")
            
            # Hilo para atender a cada cliente
            client_thread = threading.Thread(target=self.atender_cliente, args=(conn, addr))
            client_thread.start()
            
# Constructor de fault message
def mensaje_fault(code, message):
    return f"""<?xml version="1.0"?>
<methodResponse>
   <fault>
      <value>
         <struct>
            <member>
               <name>faultCode</name>
               <value><int>{code}</int></value>
               </member>
            <member>
               <name>faultString</name>
               <value><string>{message}</string></value>
            </member>
         </struct>
      </value>
   </fault>
</methodResponse> \n"""