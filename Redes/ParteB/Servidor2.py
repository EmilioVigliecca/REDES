from xmlrpc_redes import Server
import time

def resta(a,  b):
    if not isinstance(a, int):
        raise TypeError(f"El tipo de parámetro de 'a' es incorrecto, se esperaba un int y se encontró un {type(a).__name__}")
    if not isinstance(b, int):
        raise TypeError(f"El tipo de parámetro de 'b' es incorrecto, se esperaba un int y se encontró un {type(b).__name__}")
    
    return a - b

def dividir(a, b):
    if not isinstance(a, int):
        raise TypeError(f"El tipo de parámetro de 'a' es incorrecto, se esperaba un int y se encontró un {type(a).__name__}")
    if not isinstance(b, int):
        raise TypeError(f"El tipo de parámetro de 'b' es incorrecto, se esperaba un int y se encontró un {type(b).__name__}")
    
    
    return a / b

def esPrefijo (cadena1, cadena2):
    resultado = ""
    
    if not isinstance(cadena1, str):
        raise TypeError(f"El tipo de parámetro de 'cadena1' es incorrecto, se esperaba un string y se encontró un {type(cadena1).__name__}")
    if not isinstance(cadena2, str):
        raise TypeError(f"El tipo de parámetro de 'cadena2' es incorrecto, se esperaba un string y se encontró un {type(cadena2).__name__}")
    
    if (cadena2.lower().startswith(cadena1.lower())):
        resultado = (cadena1 + " es prefijo de " + cadena2)
    else:
        resultado = (cadena1 + " NO es prefijo de " + cadena2)

    return resultado

def repetir(numero, cadena):
    
    if not isinstance(numero, int):
        raise TypeError(f"El tipo de parámetro de 'numero' es incorrecto, se esperaba un int y se encontró un {type(numero).__name__}")
    if not isinstance(cadena, str):
        raise TypeError(f"El tipo de parámetro de 'cadena' es incorrecto, se esperaba un string y se encontró un {type(cadena).__name__}")
    
    return numero*cadena

def esperarDiezMas(segundos):
    if not isinstance(segundos, int):
        raise TypeError(f"El tipo de parámetro de 'segundos' es incorrecto, se esperaba un int y se encontró un {type(segundos).__name__}")
    if segundos < 0:
        raise ValueError("El parámetro 'segundos' no puede ser negativo")
    
    time.sleep(10 + segundos)
    resultado = (f"Ya pasaron los {10+segundos} segundos de espera")
    return resultado

server = Server("127.0.0.1", 5000)
server.add_method(resta)
server.add_method(dividir)
server.add_method(esPrefijo)
server.add_method(repetir)
server.add_method(esperarDiezMas)

server.serve()