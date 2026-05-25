import csv
import random

categorias = ["ELECTRONICA", "HOGAR", "ROPA"]

with open("transacciones.csv", "w", newline="") as archivo:
    writer = csv.writer(archivo)

    writer.writerow(["id", "valor_numerico", "categoria"])

    for i in range(1, 5001):

        # Algunos valores negativos para simular nulos
        if random.random() < 0.1:
            valor = random.randint(-10, -1)
        else:
            valor = random.randint(0, 100)

        # Algunas categorías vacías
        if random.random() < 0.15:
            categoria = ""
        else:
            categoria = random.choice(categorias)

        writer.writerow([i, valor, categoria])

print("Archivo generado correctamente")