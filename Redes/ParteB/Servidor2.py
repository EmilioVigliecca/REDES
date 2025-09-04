from xmlrpc_redes import Server

def resta(a,  b):
    return a - b

def dividir(a, b):
    return a / b

def esPrefijo (cadena1, cadena2):
    resultado = ""
    if (cadena2.lower().startswith(cadena1.lower())):
        resultado = (cadena1 + " es prefijo de " + cadena2)
    else:
        resultado = (cadena1 + " NO es prefijo de " + cadena2)

    return resultado


server = Server("127.0.0.1", 5000)
server.add_method(resta)
server.add_method(dividir)
server.add_method(esPrefijo)

server.serve()