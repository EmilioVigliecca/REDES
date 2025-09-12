from xmlrpc_redes import connect
import hashlib

def probar(mensaje, funcion, *args):
    print(f"\n>>> {mensaje}")
    try:
        resultado = funcion(*args)
        print(f"Resultado: {resultado}")
    except Exception as e:
        print(f"ERROR capturado: {e}")

def main():
    
    try:
        conn = connect("150.150.0.2", 5000)

        # 1. Probar suma
        probar("suma(2, 3)", conn.suma, 2, 3)
        probar("suma(2, 'tres')", conn.suma, 2, "tres")
        probar("suma(2)", conn.suma, 2)
        probar("suma(1,2,3)", conn.suma, 1, 2, 3)

        # 2. Probar multiplicar
        probar("multiplicar(2, 3, 4)", conn.multiplicar, 2, 3, 4)
        probar("multiplicar(2.5,3,4)", conn.multiplicar, 2.5, 3, 4)

        # 3. Probar es_multiplo
        probar("es_multiplo(4,2)", conn.es_multiplo, 4, 2)
        probar("es_multiplo(5,3)", conn.es_multiplo, 5, 3)

        # 4. Probar saludar
        probar("saludar()", conn.saludar)
        probar("saludar(extra)", conn.saludar, "extra")

        print("\n>>> esperarDiezMas(0) (Pausa de 10 segundos)")
        try:
            resultado = conn.esperarDiezMas(0)
            print(f"Resultado: {resultado}")
        except Exception as e:
            print(f"ERROR capturado: {e}")


        # 5. Probar eco

        probar("eco('Hola')", conn.eco, "hola")

        texto = " ".join(["palabra"] * 5000)
        print(f"\n>>> Eco con texto de {len(texto.split())} palabras")
        resultado = conn.eco(texto)
        hash_original = hashlib.md5(texto.encode()).hexdigest()
        hash_retorno = hashlib.md5(resultado.encode()).hexdigest()
        print("Coinciden los hashes?", hash_original == hash_retorno)
        print("Coinciden los tamaños?", len(texto.split()) == len(resultado.split())) 


        # 6. Probar eco con dato incorrecto
        probar("eco(1234)", conn.eco, 1234)
        
        # 7. Probar request a método no existente
        probar("funcion_secreta(666, 23, 10)", conn.funcion_secreta, 666, 23, 10)
        
        conn.socket.close()
    except Exception as e:
        print (f"ERROR: Conexión cerrada con {conn.server_address}, puerto {conn.server_port}")
   
    try:

        conn = connect("100.100.0.2", 5001)

        # 1. Probar resta
        probar("resta(10,3)", conn.resta, 10, 3)
        probar("resta(10, 'tres')", conn.resta, 10, "tres")
        probar("resta(10)", conn.resta, 10)
        probar("resta(10, 3, 2)", conn.resta, 10, 3, 2)

        # 2. Probar dividir
        probar("dividir(10,2)", conn.dividir, 10, 2)
        probar("dividir(10,0)", conn.dividir, 10, 0)
        probar("dividir(10,'2')", conn.dividir, 10, "2")

        # 3. Probar esPrefijo
        probar("esPrefijo('pre', 'prefijo')", conn.esPrefijo, "pre", "prefijo")
        probar("esPrefijo('hola', 'chau')", conn.esPrefijo, "hola", "chau")
        probar("esPrefijo(123, 'texto')", conn.esPrefijo, 123, "texto")

        # 4. Probar repetir
        probar("repetir(3, 'hola')", conn.repetir, 3, "hola")
        probar("repetir(0, 'hola')", conn.repetir, 0, "hola")
        probar("repetir(3, 5)", conn.repetir, 3, 5)

        # 5. Probar esperarDiezMas
        print("\n>>> esperarDiezMas(2) (esto tardará ~12 segundos)")
        try:
            resultado = conn.esperarDiezMas(2)
            print(f"Resultado: {resultado}")
        except Exception as e:
            print(f"ERROR capturado: {e}")

        probar("esperarDiezMas(-1)", conn.esperarDiezMas, -1)
        probar("esperarDiezMas('5')", conn.esperarDiezMas, "5")

        # 6. Probar request a método no existente
        probar("funcion_secreta(666, 23, 10)", conn.funcion_secreta, 666, 23, 10)
        

        conn.socket.close()
   
    except Exception as e:
        print (f"ERROR: Conexión cerrada con {conn.server_address}, puerto {conn.server_port}")
   

if __name__ == "__main__":
    main()
