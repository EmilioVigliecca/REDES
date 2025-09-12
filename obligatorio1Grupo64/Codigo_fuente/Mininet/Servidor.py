from xmlrpc_redes import Server
import time

def suma(a,  b):
    if not isinstance(a, int):
        raise TypeError(f"El tipo de parámetro de 'a' es incorrecto, se esperaba un int y se encontró un {type(a).__name__}")
    if not isinstance(b, int):
        raise TypeError(f"El tipo de parámetro de 'b' es incorrecto, se esperaba un int y se encontró un {type(b).__name__}")
        
    return a + b

def multiplicar(a, b, c):
    if not isinstance(a, int):
        raise TypeError(f"El tipo de parámetro de 'a' es incorrecto, se esperaba un int y se encontró un {type(a).__name__}")
    if not isinstance(b, int):
        raise TypeError(f"El tipo de parámetro de 'b' es incorrecto, se esperaba un int y se encontró un {type(b).__name__}")
    if not isinstance(c, int):
        raise TypeError(f"El tipo de parámetro de 'c' es incorrecto, se esperaba un int y se encontró un {type(b).__name__}")
    
    return a*b*c

def es_multiplo(a, b):
    if not isinstance(a, int):
        raise TypeError(f"El tipo de parámetro de 'a' es incorrecto, se esperaba un int y se encontró un {type(a).__name__}")
    if not isinstance(b, int):
        raise TypeError(f"El tipo de parámetro de 'b' es incorrecto, se esperaba un int y se encontró un {type(b).__name__}")
    
    if ((a % b == 0 ) | (b % a == 0)):
        resultado = "Sí, son múltiplos"
    else:
        resultado = "No, no son múltiplos"
    return resultado

def saludar():
    return "Hola! Chau!"

def eco(cadena):
    if not isinstance(cadena, str):
        raise TypeError(f"El tipo de parámetro de 'cadena' es incorrecto, se esperaba un string y se encontró un {type(cadena).__name__}")
    return cadena

def esperarDiezMas(segundos):
    if not isinstance(segundos, int):
        raise TypeError(f"El tipo de parámetro de 'segundos' es incorrecto, se esperaba un int y se encontró un {type(segundos).__name__}")
    if segundos < 0:
        raise ValueError("El parámetro 'segundos' no puede ser negativo")
    
    time.sleep(10 + segundos)
    resultado = (f"Ya pasaron los {10+segundos} segundos de espera")
    return resultado


server = Server("150.150.0.2", 5000)
server.add_method(suma)
server.add_method(multiplicar)
server.add_method(es_multiplo)
server.add_method(saludar)
server.add_method(eco)
server.add_method(esperarDiezMas)

server.serve()