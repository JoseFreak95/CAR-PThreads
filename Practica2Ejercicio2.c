/*
* Practica2Ejercicio2.c
* @author: Jose Carlos Fuentes Angulo
* @date: 27 Abril 2020
* @description: Programa en C para encontrar el valor mínimo de un vector
* de forma paralela. 
*
* La estrategia para paralelizar el problema será, dividir el vector en tantos 
* subvectores como hilos haya y asignar cada subvector a cada hilo. 
* De esta forma cada hilo buscará el mínimo del subvector y al encontrarlo, 
* el flujo de programa principal comparará el mínimo de cada subvector 
* para encontrar el mínimo.
*/

/*Incluimos las cabeceras necesarias*/
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

/*
* @description: Funcion para generar vectores de enteros aleatorios
* @params: Tamaño del vector
* @return: Puntero al vector creado
*/
double* crearVectorAleatorio(int tam){
	double* vec = malloc(sizeof(double)*tam);
	for(int i = 0; i<tam; i++)
		vec[i] = rand() % 66;
	return vec;
}

/*
* Defimos una estructura que guardará los parámetros necesarios para trabajar con funciones hebradas.
* Los elementos son:
*	- Un vector de enteros
*	- Una variable que determina el índice de inicio del subvector que le corresponde al hilo
*	- Una variable que determina el índice de fin del subvector que le corresponde al hilo
*	- Una variable que almacenará el mínimo de cada subvector.
*/
typedef struct tarea{
	double* vector;	
	int ini;
	int fin;
	double resultadoLocal;
} tarea;

/*
* Defimos una estructura que guardará los parámetros necesarios para balancear la carga
* en el caso de que a la hora de dividir los elementos en paquetes para cada hilo esta división
* no sea entera.
* Los elementos son:
*	- Una varaible para establecer cual es el índice de inicio actual
*	- Una variable para establecer cual es el índice de fin actual
*	- Una variable que representa el número de elementos de cada subvector (cociente de la división)
*	- Una variable que representa el número de elementos sobrantes del reparto (resto de la divisón)
*/
typedef struct loadBalancer
{
	int currentIni;
	int currentFin;
	int pack;
	int offset;
} Balanceador;

/*
* @description: Función que busca el mínimo valor dentro de un vector
* @params: Puntero a vacío que contiene los parámetros de la función
* @return: Puntero a vacío
*/
void* minimunFinder(void* args){
	tarea* deberes = (tarea*) args;
	deberes->resultadoLocal =__DBL_MAX__;
	for(int i = deberes->ini; i<deberes->fin; i++){
		if((deberes->vector)[i] < deberes->resultadoLocal)
			deberes->resultadoLocal = (deberes->vector)[i];
	}
	pthread_exit(NULL);
}

/*
* @description: Función que reparte los elementos sobrantes de la división de paquetes.
* Este balanceador de carga añade los elementos sobrantes a los paquetes, uno a cada uno
* hasta que no queden elementos.
* @params: Puntero a la estructura de parámetros con el vector de elementos, 
			entero con el índice del paquete que se está creando, 
			puntero a la estructura de parámetros del balanceador,
			puntero a entero que guarda una copia del valor de elementos restantes, 
			flag de control para determinar la estrategia de balanceo
* @return: Puntero a vacío
*/
void loadBalancing(tarea * deberes, int index, Balanceador * balanceador, int * offsetCopy, bool needBalancing)
{
	if (needBalancing)	//Se comprueba el flag para determinar la estratecia de balanceo
	{				  	//Estrategia de balanceo para división no exacta
		if ((*offsetCopy) != 0)
		{
			deberes->ini = balanceador->currentIni;				//Se establece el índice de inicio para el subvector "index"
			balanceador->currentFin += (balanceador->pack + 1);	//Se actualiza el valor del índice actual de fin con el número de elementos del paquete más 1
			deberes->fin = balanceador->currentFin;				//Se establece el índice de fin para el subvector "index"
			balanceador->currentIni = balanceador->currentFin;	//Se actualiza el valor del índice actual de inicio con el valor del ínidice actual de fin
																//de esta forma, el fin de un paquete es el inicio del siguiente y asi
			(*offsetCopy)--;
		}
		else
		{
			deberes->ini = balanceador->currentIni;				//Se actualiza el índice de fin del último paquete creado.
			balanceador->currentFin += (balanceador->pack);		//Esto se debe a que como no quedan elemento que repartir
			deberes->fin = balanceador->currentFin;				//la longitud del subvector es igual al tamaño del paquete.
			balanceador->currentIni = balanceador->currentFin;
		}
	}
	else //Estrategia de balanceo para división exacta
	{
		deberes->ini = index*balanceador->pack;
		deberes->fin = index*balanceador->pack + balanceador->pack;
	}
}

/*
* @description: Función principal
* @params: Entero que representa el de argumentos, Puntero al array de argumentos
* @return: Entero
*/
int main(int argc, char* argv[]){
	
	//Actualizamos la semilla para que en cada ejecución se generen valores distintos
	srand(time(NULL));

	double resultadoGlobal = __DBL_MAX__;	//Variable global que guardará el mínimo valor del vector
	
	if(argc != 3){
		printf("Llamada: ./prog Longitud nHilos\n");
	}else{
		int longitud = atoi(argv[1]);	//Capturamos el argumento correspondiente a la longitud del vector
		int nHilos = atoi(argv[2]);		//Capturamos el argumento correspondiente al número de hilos
		double* vector = crearVectorAleatorio(longitud);	//Creamos un vector aleatorio

		printf("Elementos del vector: ");
		for(int j = 0; j< longitud; j++)
		{
			printf("%.2f\t", vector[j]);
		}

		printf("\n");

		//Creamos un puntero a la estructura de parámetros (array de estructuras).
		//El puntero debe tener el tamaño de la estructura por el número de  hilos
		//ya que en tiempo de ejecución se crearan tantas estructuras como hilos
		tarea* deberes = malloc(sizeof(tarea)*nHilos);
		
		//Creamos un array con los hilos
		pthread_t* hilos = malloc(sizeof(pthread_t)*nHilos);
		
		//Declaramos una varaible tipo estructura de parámetros del balanceador
		Balanceador balanceador;

		//Inicializamos los parámetros del balanceador de carga
		balanceador.currentIni = 0;
		balanceador.currentFin = 0;
		balanceador.pack = longitud / nHilos;
		balanceador.offset = longitud % nHilos;
		
		//Copiamos el valor de elementos restantes para restarlo durante el reparto
		int offsetCopy = balanceador.offset;

		//Rellenamos las estructuras de parámetros que se asignaran a cada hilo.
		//En cada iteración, asignamos el vector de elementos y comprobamos que existan
		//elementos restantes o no para elegir la estrategia de reparto, inicializamos
		//la variable que guardará el mínimo de cada subvector y creamos el hilo
		for(int i = 0; i<nHilos; i++){
			deberes[i].vector = vector;
			
			loadBalancing(&deberes[i], i, &balanceador, &offsetCopy, (balanceador.offset != 0));
			
			deberes[i].resultadoLocal = 0;
			pthread_create(&hilos[i], 0, minimunFinder, &deberes[i]);		
		}

		//Estructura iterativa para controlar el flujo de programa principal.
		//Con cada hilo que termina, se comprueba el mínimo de cada subvector y
		//se actualiza la variable local con el valor más pequeño
		for(int i = 0; i<nHilos; i++){
			pthread_join(hilos[i], 0);	
			if(deberes[i].resultadoLocal < resultadoGlobal)
			{
				resultadoGlobal = deberes[i].resultadoLocal;
			}
		}

		//Imprimimos el resultado por pantalla
		printf("El mínimo elemento del vector es: %.2f\n", resultadoGlobal);

		//Liberamos memoria alocada
		free(hilos);
		free(deberes);
		free(vector);
	}
	return 0;
}