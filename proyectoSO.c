#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h> // Para usar strcpy en las categorias

#define MAX_FILAS 100000

//Estructura para almacenar los datos de una transacción o tarea
typedef struct {
    int id;
    double valor_numerico;
    char categoria[50];
} Transaccion;

//Estructura que recibirá cada hilo como argumento
typedef struct {
    Transaccion *datos;
    int inicio;
    int fin;
    int id_hilo;
    double tiempo_ejecucion; // Para reportar el tiempo por hilo
} ThreadArgs;

// Variables globales para los datos
Transaccion dataset[MAX_FILAS];
int total_lineas = 0;

// Inicializamos el mutex aqui para no tener que tocar el main
pthread_mutex_t mutex_resultados = PTHREAD_MUTEX_INITIALIZER;

// Contadores para guardar los resultados finales
int total_nulos_limpiados = 0;
int total_categorias_imputadas = 0;

// Constantes que nos sirven para calcular la normalizacion y la moda
const double VALOR_MIN = 0.0;
const double VALOR_MAX = 100.0;
const char MODA_CATEGORIA[] = "MODA_EJEMPLO";

// Función que simula el procesamiento de una fila
void procesar_fila(Transaccion *t) {
    
    // Limpiamos los nulos, asumiendo que los valores negativos son nulos o errores
    if (t->valor_numerico < 0.0) {
        t->valor_numerico = 0.0; 
        
        // Usamos el mutex para que los hilos no choquen al sumar al mismo tiempo
        pthread_mutex_lock(&mutex_resultados);
        total_nulos_limpiados++;
        pthread_mutex_unlock(&mutex_resultados);
    }

    // Imputamos con la moda si vemos que el string de la categoria esta vacio
    if (t->categoria[0] == '\0') {
        strcpy(t->categoria, MODA_CATEGORIA); 
        
        // Bloqueamos otra vez para actualizar este contador global
        pthread_mutex_lock(&mutex_resultados);
        total_categorias_imputadas++;
        pthread_mutex_unlock(&mutex_resultados);
    }

    // Normalizamos el valor con la formula para que quede en escala de 0 a 1
    if (VALOR_MAX > VALOR_MIN) {
        t->valor_numerico = (t->valor_numerico - VALOR_MIN) / (VALOR_MAX - VALOR_MIN);
    }
}

//Función que ejecutarán los hilos en el Modo Paralelo
void* funcion_hilo(void *arg) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    ThreadArgs *args = (ThreadArgs *)arg;

    printf("[Hilo %d] Procesando desde la fila %d hasta %d...\n", args->id_hilo, args->inicio, args->fin);

    for (int i = args->inicio; i < args->fin; i++) {
        procesar_fila(&args->datos[i]);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    // Calcular tiempo en segundos
    args->tiempo_ejecucion = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("[Hilo %d] Finalizado en: %.5fs\n", args->id_hilo, args->tiempo_ejecucion);

    pthread_exit(NULL);
}

//Lógica para el Modo Secuencial
void ejecutar_modo_secuencial() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("\n*** Iniciando Modo Secuencial (1 solo hilo) ***\n");
    for (int i = 0; i < total_lineas; i++) {
        procesar_fila(&dataset[i]);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double tiempo_total = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("TIEMPO TOTAL SECUENCIAL: %.5fs\n", tiempo_total);
}

//Lógica para el Modo Paralelo
void ejecutar_modo_paralelo(int num_hilos) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("\n*** Iniciando Modo Paralelo (%d hilos) ***\n", num_hilos);

    pthread_t hilos[num_hilos];
    ThreadArgs argumentos[num_hilos];

    // División de la carga de trabajo en bloques de filas
    int filas_por_hilo = total_lineas / num_hilos;
    int residuo = total_lineas % num_hilos; // Por si la división no es exacta
    // Creación de hilos
    for (int i = 0; i < num_hilos; i++) {
        argumentos[i].datos = dataset;
        argumentos[i].inicio = i * filas_por_hilo;
        // El último hilo se lleva el residuo de filas si existe
        argumentos[i].fin = (i == num_hilos - 1) ? (argumentos[i].inicio + filas_por_hilo + residuo) : (argumentos[i].inicio + filas_por_hilo);
        argumentos[i].id_hilo = i + 1;
        argumentos[i].tiempo_ejecucion = 0.0;

        if (pthread_create(&hilos[i], NULL, funcion_hilo, (void *)&argumentos[i]) != 0) {
            perror("Error al crear el hilo");
            exit(EXIT_FAILURE);
        }
    }

    // Unión de hilos, se espera a que todos terminen
    for (int i = 0; i < num_hilos; i++) {
        pthread_join(hilos[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double tiempo_total = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("TIEMPO TOTAL PARALELO (%d hilos): %.5fs\n", num_hilos, tiempo_total);
}

//Hilo Principal (Main): Lectura de datos y control
int main(int argc, char *argv[]) {
    // Simulación de lectura de archivo, supongamos que leemos miles de transacciones
    total_lineas = 50000;
    for(int i = 0; i < total_lineas; i++) {
        dataset[i].id = i;
        dataset[i].valor_numerico = (double)(rand() % 100);
    }
    printf("Archivo cargado con éxito. Total filas: %d\n", total_lineas);

    // Ejecución de pruebas según los requerimientos solicitados
    ejecutar_modo_secuencial();

    ejecutar_modo_paralelo(3); // Requerimiento del enunciado (3 hilos)
    ejecutar_modo_paralelo(4); // Requerimiento del enunciado (4 hilos)

    // Impresion de los resultados globales obtenidos
    printf("\n*** RESULTADOS DEL PROCESAMIENTO ***\n");
    printf("Total de nulos limpiados: %d\n", total_nulos_limpiados);
    printf("Total de categorias imputadas (vacios reemplazados): %d\n", total_categorias_imputadas);

    return 0;
}
