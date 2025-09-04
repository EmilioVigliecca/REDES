from xmlrpc_redes import Server

def suma(a,  b):
    return a + b

def multiplicar(a, b, c):
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