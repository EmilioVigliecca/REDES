from xmlrpc_redes import connect

def probar(mensaje, funcion, *args):
    print(f"\n>>> {mensaje}")
    try:
        resultado = funcion(*args)
        print(f"Resultado: {resultado}")
    except Exception as e:
        print(f"ERROR capturado: {e}")

def main():
    conn = connect("127.0.0.1", 5000)

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

if __name__ == "__main__":
    main()
