from xmlrpc_redes import connect
import hashlib

conn = connect("127.0.0.1", 5000)

# Generar un string con 20.000 palabras
texto = " ".join(["palabra"] * 25000)

print("Texto generado con", len(texto.split()), "palabras")

resultado = conn.eco(texto)

# Verificar
hash_original = hashlib.md5(texto.encode()).hexdigest()
hash_retorno = hashlib.md5(resultado.encode()).hexdigest()

print("Coinciden los hashes?", hash_original == hash_retorno)

print("Original: ", len(texto.split()))

print("Resultado:", len(resultado.split()))
