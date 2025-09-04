from xmlrpc_redes import connect 


conn = connect("127.0.0.1", 5000)



while(True):
    print("Opciones disponibles:")
    print ("(multiplicar es con tres parametros, el resto con dos)")
    try: 
        metodos = conn.listarMetodos()
        metodos = metodos.split(",")
        if metodos[0] == "listarMetodos": 
            metodos = metodos[1:] # Sacar listarMetodos
            metodos = [m.strip() for m in metodos] # Saca los espacios así queda más prolijo 
            
        metodos = ", ".join(metodos)

        print(metodos)
    except Exception as e:
        print("Error al recuperar métodos:", e)
        exit(1)
        
    
    #for metodo in metodos:
    #    print("-", metodo)
    print("Para salir escriba exit")
    
    
    entrada = input("Ingrese operacion:")
    if (entrada.lower() == "exit"):
        conn.socket.close()
        break
    try:
        resultado = eval(f"conn.{entrada}")
        print(f"El resultado fue: {resultado}\n")
    except Exception as e:
        print(f"El resultado fue: error - {e}")