from xmlrpc_redes import connect

conn = connect("127.0.0.1", 5000)

while(True):
    print("Opciones disponibles:")
    try:
        metodos = conn.listarMetodos()
        metodos = metodos.split(",")
        metodos = [m.strip() for m in metodos]
        metodos = ", ".join(metodos)
        print(metodos + "\n")

    except Exception as e:
        print("Error al recuperar métodos:", e)
        exit(1)

    print("Para salir escriba exit\n")
    entrada = input("Ingrese operacion:")
    if (entrada.lower() == "exit"):
        conn.socket.close()
        break
    
    try:
        # Separar el nombre del método y los argumentos
        partes = entrada.split('(', 1)
        nombre_metodo = partes[0]
        
        if len(partes) > 1:
            args_str = partes[1].strip(')')
            args_list = []
            if args_str:
                for a in args_str.split(','):
                    valor = a.strip()
                    # Verifica si el argumento es una cadena (comienza y termina con comillas)
                    #if (valor.startswith("'") and valor.endswith("'")) or (valor.startswith('"') and valor.endswith('"')):
                    #    args_list.append(valor.strip("'").strip('"'))
                    #elif valor.isdigit():
                        # Si no, lo trata como un entero
                    #    args_list.append(int(valor))
                    #else:
                    #    args_list.append(valor)
         
                    # Si está entre comillas, es string literal
                    if (valor.startswith("'") and valor.endswith("'")) or (valor.startswith('"') and valor.endswith('"')):
                        args_list.append(valor.strip("'").strip('"'))
                    else:
                        # Intento primero int, luego float, y si nada funciona lo dejo como string
                        try:
                            args_list.append(int(valor))
                        except ValueError:
                            try:
                                args_list.append(float(valor))
                            except ValueError:
                                args_list.append(valor)  # se envía como string                            
                args = args_list
            else:
                args = []
        else:
            args = []
#esto es una cadena de soluciones, primero daba un error rarisimo si la entrada no tenia parentesis
#porque eval ejecutaba como codigo conn.ejemplo y se rompia
#Para solucionar esto separe la entrada entre nombre de metodo y parametros

#Ahi daba error si ponias una funcion sin parametros "resta()" porque el codigo tomaba como int al string ''
#Otro error que habia era que no tomaba ningun valor string (no se podia ejecutar esPrefijo)
#solucionado con if valor.startswith("'")
            
        # Llamar al método dinámicamente
        metodo_remoto = getattr(conn, nombre_metodo)
        resultado = metodo_remoto(*args)
        
        print(f"\nResultado: {resultado}\n")
    except AttributeError:
        print("\nERROR: El método ingresado no existe.\n")
    except Exception as e:
        print(f"\nERROR: - {e}\n")