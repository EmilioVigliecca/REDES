Primeramente, la carpeta Local y Mininet contienen el mismo código. La única diferencia es que una está destinada
al uso local (por ende, las conexiones se limitan al puerto 127.0.0.1), mientras que la otra a la prueba en mininet 
(con las direcciones correspondientes a la topología de la consigna).

Para ejecutar el sistema se deberá:

Primero ejecutar en dos terminales separadas Servidor.py y Servidor2.py.
Los servidores despliegan en la terminal cuando se estableció una conexión así como cuando esta finalizó.

Luego, ejecutar cualquiera de los .py (sin ser la biblioteca xmlrpc_redes) para realizar las pruebas.
El Cliente.py engloba la mayoría de casos de prueba e imprime en consola lo que se quiso ejecutar, el resultado ó un error en caso de que algo extraño haya sucedido.
Adicionalmente, se pueden ejecutar TestPalabraGigante.py, test_parse.py y test_OtroError.py para probar
el eco con un string con más de 20000 palabras, el correcto funcionamiento del parser XML y otros posibles errores (content-length incorrecto), respectivamente.
