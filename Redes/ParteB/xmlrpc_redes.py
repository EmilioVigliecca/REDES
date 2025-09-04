import threading
import socket
import xml.etree.ElementTree as ET

class Client:

    def __init__(self, address, port):
        self.server_address = address
        self.server_port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((address, port)) #Es tupla, no sacar segundo parentesis

    def _get_xml_value(self, arg):

        #Determina el tipo de un argumento y lo envuelve en la etiqueta XML adecuada.

        if isinstance(arg, int):
            return f"<value><int>{arg}</int></value>"
        elif isinstance(arg, str):
            return f"<value><string>{arg}</string></value>"
        else:
            raise TypeError(f"Unsupported argument type: {type(arg)}")

    def __getattr__(self, nombre_metodo):
        #Esta funcion se llama cuando se invoca un metodo que no esta definido: 
        
        def metodo_remoto(*args):
            
            # armar el XML:
            
            parametros = "".join(
                f"<param>{self._get_xml_value(a)}</param>" for a in args
            )             
            xml = f"""<?xml version="1.0"?>
                <methodCall>
                    <methodName>{nombre_metodo}</methodName>
                    <params>{parametros}</params>
                </methodCall>"""

            request = (
                f"POST /RPC2 HTTP/1.0\r\n"
                f"User-Agent: Python-XMLRPC\r\n" #No se si usar este
                f"Host: {self.server_address}\r\n"
                f"Content-Type: text/xml\r\n"
                f"Content-Length: {len(xml)}\r\n\r\n"
                f"{xml}"
            )
            # Enviarlo al servidor
            self.socket.sendall(request.encode()) 
            
            # Recibir respuesta
            data = self.socket.recv(4096)
            respuesta_xml = data.decode()

            try:
                root = ET.fromstring(respuesta_xml)
            except ET.ParseError as e:
                return f"Error al parsear XML: {e}"
            
            if root.tag == "methodResponse":
                param_node = root.find("params/param/value")
                if param_node is not None:
                    return param_node[0].text
                
                
                fault_string = root.find("fault/value/struct/member[name='faultString']/value/string")
                return f"Error: {fault_string.text if fault_string is not None else 'desconocido'}"
            else:
                return "Respuesta inválida"
        return metodo_remoto
            
def connect(address, port):
    return Client(address, port)
    
class Server:
    def __init__(self, address, port):
        self.address = address
        self.port = port
        self.metodos = {}
        self.add_method(self.listarMetodos)
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.bind((address, port)) #recibe una tupla (address, port)
        self.socket.listen(5)

        print(f"Servidor escuchando en {self.address}:{self.port}")

    def add_method(self, proc1):
            self.metodos[proc1.__name__] = proc1

    #Funcion que se ejecuta por cada thread:
    def atender_cliente(self, conn, addr):

        print(f"Atendiendo a: {addr}")
        try:
            while (True):
                # Recibir datos completos del cliente.
                data = conn.recv(4096).decode()
                
                # Separar las cabeceras del cuerpo del mensaje HTTP.
                # La parte del cuerpo contiene el XML.
                header, xml_body = data.split('\r\n\r\n', 1)
                
                # Procesar el XML del cuerpo. (<methodCall en mensaje del cliente>)
                try:
                    root = ET.fromstring(xml_body)
                except ET.ParseError as e:
                    respuesta_xml = mensaje_fault(1, f"Error de parseo XML")
                    conn.sendall(respuesta_xml.encode())
                    return
                
                # Extraer el nombre del método.
                method_name = root.find('methodName').text
                
                # Extraer los parámetros navegando por la estructura anidada.
                params = []
                #try:
                params_node = root.find('params')    
                if params_node is not None:
                    for param_node in params_node.findall('param'):
                        value_node = param_node.find('value')
                        if value_node is not None:
                            # Revisa si el valor es un entero
                            int_node = value_node.find('int')
                            string_node = value_node.find('string')
                            if int_node is not None:
                                params.append(int(int_node.text))
                            elif string_node is not None:
                                params.append(string_node.text)
                            else:
                                raise ValueError("Tipo de parámetro no válido")

                #identificar que funcion pidio:
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
                        else: # Se asume que es string para los demás casos como 'listarMetodos'
                            respuesta_xml = f"""<?xml version="1.0"?>
        <methodResponse>
        <params>
            <param>
                <value><string>{result}</string></value>
            </param>
        </params>
        </methodResponse>"""
                    except Exception as e:
                        respuesta_xml = mensaje_fault(3, f"Error en parámetros del método invocado.")

                conn.sendall(respuesta_xml.encode())
#hasta aca iria el while true
        except Exception as e:
            respuesta_xml = mensaje_fault(4, f"Error interno en la ejecución del método: {e}")
            conn.sendall(respuesta_xml.encode())
        finally:
            conn.close()
            print(f"Conexión terminada con {addr}")

        

    def serve(self):
        while True:
            # INLCUIR THREADS PARA QUE PUEDA ATENDER A MAS DE UN CLIENTE A LA VEZ
            conn, addr = self.socket.accept()
            print(f"Conexión aceptada de {addr}")
            

            client_thread = threading.Thread(target=self.atender_cliente, args=(conn, addr))
            
            # Inicia el hilo. Esto permite que el servidor vuelva a la línea `accept()`.
            client_thread.start()
            
    def listarMetodos(self):
        return ", ".join(self.metodos.keys())



#Constructor de Fault Examples
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
</methodResponse>"""
