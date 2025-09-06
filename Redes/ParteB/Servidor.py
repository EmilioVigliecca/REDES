from xmlrpc_redes import Server

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
    if ((a % b == 0 ) | (b % a == 0)):
        resultado = "Sí, son multiplos"
    else:
        resultado = "No, no son multiplos"
    return resultado

server = Server("127.0.0.1", 5000)
server.add_method(suma)
server.add_method(multiplicar)
server.add_method(es_multiplo)
server.serve()