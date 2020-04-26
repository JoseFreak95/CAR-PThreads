/*
* Practica2Ejercicio4A.c
* @author: Jose Carlos Fuentes Angulo
* @date: 27 Abril 2020
* @description: Programa en C para realizar la suma de los elementos de una matriz
* de forma paralela. 
*
* La estrategia para paralelizar el problema será seguir una estrategia de reparto
* por filas, es decir, cada hilo se encargará de sumar los elementos de las filas que
* correspondan en el reparto.
*/

/*Incluimos las cabeceras necesarias*/
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

/*
* @description: Funcion para generar matrices de enteros aleatorios
* @params: Tamaño del vector
* @return: Puntero al vector creado
*/
int** crearMatrizAleatoria(int filas, int columnas){
	int** matrix = malloc(sizeof(int*)*filas);
	for(int i = 0; i<filas; i++){
		matrix[i] = malloc(sizeof(int)*columnas);  
    }

	for(int i = 0; i<filas; i++){
		for(int j = 0; j<columnas; j++){
			matrix[i][j] = rand() % 66;
		}
    }

	return matrix;
}

/*
* Defimos una estructura que guardará los parámetros necesarios para trabajar con funciones hebradas.
* Los elementos son:
*	- Una matriz de enteros
*	- Una variable que determina el índice de la fila de inicio que le corresponde al hilo
*	- Una variable que determina el índice de la fila de fin que le corresponde al hilo
*	- Una variable que determina el número de columnas
*	- Una varaible que guardará el total de la suma de cada hilo
*/
typedef struct tarea{
	int** matriz;	
	int filaIni;
	int filaFin;
	int columnas;
	int resultadoLocal;
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
* @description: Función que realiza el producto escalar de dos vectores
* @params: Puntero a vacío que contiene los parámetros de la función
* @return: Puntero a vacío
*/
void* sumaElementosMatriz(void* args){
	tarea* deberes = (tarea*) args;

	for(int i = deberes->filaIni; i < deberes->filaFin; i++)
	{
		for (int j = 0; j < deberes->columnas; j++)
		{
			deberes->resultadoLocal += (deberes->matriz)[i][j];
		}
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
			deberes->filaIni = balanceador->currentIni;			//Se establece el índice de inicio para el subvector "index"
			balanceador->currentFin += (balanceador->pack + 1);	//Se actualiza el valor del índice actual de fin con el número de elementos del paquete más 1
			deberes->filaFin = balanceador->currentFin;			//Se establece el índice de fin para el subvector "index"
			balanceador->currentIni = balanceador->currentFin;	//Se actualiza el valor del índice actual de inicio con el valor del ínidice actual de fin
																//de esta forma, el fin de un paquete es el inicio del siguiente y asi
			(*offsetCopy)--;
		}
		else
		{
			deberes->filaIni = balanceador->currentIni;			//Se actualiza el índice de fin del último paquete creado.
			balanceador->currentFin += (balanceador->pack);		//Esto se debe a que como no quedan elemento que repartir
			deberes->filaFin = balanceador->currentFin;			//la longitud del subvector es igual al tamaño del paquete.
			balanceador->currentIni = balanceador->currentFin;
		}
	}
	else //Estrategia de balanceo para división exacta
	{
		deberes->filaIni = index*balanceador->pack;
		deberes->filaFin = index*balanceador->pack + balanceador->pack;
	}
}

void liberarMatriz(int** matriz, int filas)
{
	for(int i = 0; i < filas; i++)
	{
		free(matriz[i]);
	}
	free(matriz);
}

/*
* @description: Función principal
* @params: Entero que representa el de argumentos, Puntero al array de argumentos
* @return: Entero
*/
int main(int argc, char* argv[]){
	
	//Actualizamos la semilla para que en cada ejecución se generen valores distintos
	srand(time(NULL));

	//Declaramos la variable global que almacenará la suma de los elementos de la matriz
	int resultadoGlobal = 0;
	
	if(argc != 4){
		printf("Llamada: ./prog Filas Columnas nHilos\n");
	}else{
		int filas = atoi(argv[1]);		//Capturamos el argumento correspondiente a las filas de la matriz
		int columnas = atoi(argv[2]);	//Capturamos el argumento correspondiente a las columnas de la matriz
		int nHilos = atoi(argv[3]);		//Capturamos el argumento correspondiente al número de hilos
		int** matrix = crearMatrizAleatoria(filas, columnas);	//Creamos una matriz aleatoria

		//Imprimimos la matriz por pantalla para ver su contenido
		printf("Matriz:\n");
		for (int i = 0; i < filas; i++)
		{
			for (int j = 0; j < columnas; j++)
			{
				printf("%d\t", matrix[i][j]);
			}
			printf("\n");
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
		balanceador.pack = filas / nHilos;
		balanceador.offset = filas % nHilos;
		
		//Copiamos el valor de elementos restantes para restarlo durante el reparto
		int offsetCopy = balanceador.offset;

		//Rellenamos las estructuras de parámetros que se asignaran a cada hilo.
		//En cada iteración, asignamos la matriz de elementos, asignamos el número de
        //columnas y comprobamos que existan elementos restantes o no para elegir la estrategia 
        //de reparto de filas, inicializamos la variable del resultado y creamos el hilo
		for(int i = 0; i<nHilos; i++){
			deberes[i].matriz = matrix;
			
			loadBalancing(&deberes[i], i, &balanceador, &offsetCopy, (balanceador.offset != 0));

			deberes[i].columnas = columnas;
			deberes[i].resultadoLocal = 0;
			
			pthread_create(&hilos[i], 0, sumaElementosMatriz, &deberes[i]);		
		}

		//Estructura iterativa para controlar el flujo de programa principal.
		//El búcle iterará hasta que todos los hilos hayan terminado su tarea.
		for(int i = 0; i<nHilos; i++){
			pthread_join(hilos[i], 0);
			resultadoGlobal += deberes[i].resultadoLocal;
		}

		//Imprimimos el resultado por pantalla
		printf("La suma de los elementos de la matriz es: %d\n", resultadoGlobal);

		//Liberamos memoria alocada
		free(hilos);
		free(deberes);
		liberarMatriz(matrix, filas);
	}
	return 0;
}