from xmlrpc_redes import connect 


conn = connect("127.0.0.1", 5000)
while(True):
    print("opciones: suma(a,b), multiplicar(a,b,c), es_multiplo(a,b), exit")
    entrada = input("ingrese operacion:")
    if (entrada == "exit"):
        conn.socket.close()
        break
    try:
        #entrada = input("ingrese operacion:")
        
        resultado = eval(f"conn.{entrada}")
        print(f"El resultado fue: {resultado}")
    except Exception as e:
        print(f"El resultado fue: error")
