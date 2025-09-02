from xmlrpc_redes import Server

def resta(a,  b):
    return a + b

def dividir(a, b):
    return a*b



server = Server("127.0.0.1", 5000)
server.add_method(resta)
server.add_method(dividir)

server.serve()