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
    conn = connect("127.0.0.1", 5000)

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

    # 5. Probar eco con texto largo

    probar("eco('Hola')", conn.eco, "hola")

    texto = " ".join(["palabra"] * 25000)
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

if __name__ == "__main__":
    main()
