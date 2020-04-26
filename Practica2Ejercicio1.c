/*
* Practica2Ejercicio1.c
* @author: Jose Carlos Fuentes Angulo
* @date: 27 Abril 2020
* @description: Programa en C para restar el total de un vector de ingresos y el total
* un vector de gastos de forma paralela.
*
* La estrategia de paralelización consistirá en asignar un vector a cada hilo.
* Dado que es requisito que se usen solo dos hilos, cada hilo hará la suma de los
* elementos de su vector asignado. Posteriormente, el flujo de programa principal
* restará el valor total de ambos vectores (ingresos-gastos) para encontrar la diferencia.
*/

/*Incluimos las cabeceras necesarias*/
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

/*
* @description: Funcion para generar vectores de enteros aleatorios
* @params: Tamaño del vector
* @return: Puntero al vector creado
*/
int * crearVectorAleatorio(int tam)
{
    int* vec = malloc(sizeof(int) * tam);
    for (int i = 0; i < tam; i++)
        vec[i] = rand() % 66;
    return vec;
}

/*
* Defimos una estructura que guardará los parámetros necesarios para trabajar con funciones hebradas.
* Los elementos son:
*	- Los dos vectores de enteros
*	- Dos variables para guardar los resultados locales de cada función hebrada
*	- Longitud de los vectores
*/
typedef struct banco
{
	int * ingresos;
	int * gastos;
	int resultadoIngresos;
	int resultadoGastos;
	int longitud;
} Bank;

/*
* @description: Función que suma el vector de ingresos y guarda el total en una variable
* @params: Puntero a vacío que contiene los parámetros de la función
* @return: Puntero a vacío
*/
void* sumaIngresos(void* args)
{
	Bank* bancoLocal = (Bank*) args;
	for(int i = 0; i < (bancoLocal->longitud); i++){
		bancoLocal->resultadoIngresos += (bancoLocal->ingresos)[i];
	}
	pthread_exit(NULL);
}

/*
* @description: Función que suma el vector de gastos y guarda el total en una variable
* @params: Puntero a vacío que contiene los parámetros de la función
* @return: Puntero a vacío
*/
void* sumaGastos(void* args)
{
	Bank* bancoLocal = (Bank*) args;
	for(int i = 0; i < (bancoLocal->longitud); i++){
		bancoLocal->resultadoGastos += (bancoLocal->gastos)[i];
	}
	pthread_exit(NULL);
}

/*
* @description: Función principal
* @params: Entero que representa el de argumentos, Puntero al array de argumentos
* @return: Entero
*/
int main(int argc, char *argv[])
{
	//Actualizamos la semilla para que en cada ejecución se generen valores distintos
	srand(time(NULL));

	int resultadoGlobal = 0;	//Variable global que guardará el resultado de restar el total
								//ingresos con el total de gastos

	if (argc != 2)
	{
	    printf("Llamada: ./prog Longitud\n");
	}
	else
	{
		//Creamos un puntero a la estructura de parámetros
		Bank* banco = malloc(sizeof(Bank));
		//Creamos un puntero un vector de hilos.
        pthread_t *hilos = malloc(sizeof(pthread_t) * 2);

		//Inicializamos los parámetros de la estructura
		banco->longitud = atoi(argv[1]);
        banco->ingresos = crearVectorAleatorio(banco->longitud);
        banco->gastos = crearVectorAleatorio(banco->longitud);
		banco->resultadoIngresos = 0;
		banco->resultadoGastos = 0;

		//Creamos creamos los hilos y asignamos las funciones hebradas a cada hilo
		pthread_create(&hilos[0], 0, sumaIngresos, banco);
		pthread_create(&hilos[1], 0, sumaGastos, banco);

		//Declaramos las funciones join para cada uno de los hilos creamos.
		//Estas funciones sirven para frenar la ejecución de la función main()
		//hasta que terminen de ejecutarse los hilos.
		pthread_join(hilos[0], 0);
		pthread_join(hilos[1], 0);

		//Obtenemos el valor diferencial entre el total de ingresos y el total de gastos
		resultadoGlobal = banco->resultadoIngresos - banco->resultadoGastos;

		//Imprimimos los resultados por pantalla
		printf("Total vector ingresos: %d\n", banco->resultadoIngresos);
		printf("Total vector gastos: %d\n", banco->resultadoGastos);
        printf("Diferencia: %d\n", resultadoGlobal);

		//Liberamos la memoria alocada
        free(hilos);
        free(banco);
    }
    return 0;
}