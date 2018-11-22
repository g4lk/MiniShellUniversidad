#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>			
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

#define MAX_COCHES 200

//Variables globales

int plazasDisponibles; //Numero de plazas disponibles en el parking
int plazas= 7; //Numero de plazas total.
int* parking; 
int idCoche=1;
int idCamion=100;
pthread_cond_t dosHuecos;
pthread_cond_t saleCoche;
pthread_mutex_t entradaSalidaParking;

//Reservamos espacio para el parking y confirmamos por pantalla.
void inicializarParking(void){
	int i;
	parking = malloc(plazas*sizeof(int));
	for (i=0;i<plazas;i++){
		parking[i] = 0;
	}
	plazasDisponibles=plazas;
	printf("Parking creado!\n");
}

// Inicializamos mutex entrada y salida y condiciones.
void inicializarEntradaSalida(void){
	pthread_mutex_init(&entradaSalidaParking,NULL);
	pthread_cond_init(&saleCoche,NULL);
	pthread_cond_init(&dosHuecos,NULL);
}

void escribirParking(void){
	int i;
	printf("Parking:");
	for (i=0;i<plazas;i++){
		if (i==plazas-1){
			printf(" [%d]\n",parking[i]);
		}
		else{
			printf(" [%d]",parking[i]);
		}
	}
}
//Metodo que asigna un camion a dos plazas
void *asignarPlazaCamion(void *id){
	int j;
	pthread_mutex_lock(&entradaSalidaParking);
	if(plazasDisponibles <= 1){ //Si esta lleno el parking
		printf("El Parking no puede almacenar el camion, el camion esperará a entrar\n");
		pthread_cond_wait(&dosHuecos,&entradaSalidaParking);
	}
	for (j=0; j<plazas -1 ; j++){ //Busco las primeras plazas disponibles
		if(parking[j]==0 && parking[j+1] == 0){  
			plazasDisponibles-= 2;
			printf("ENTRADA: El camion %d aparca en %d. Plazas libres: %d\n",idCamion,j,plazasDisponibles);
			parking[j] = idCamion;
			parking[j+1] = idCamion; //Ocupo las plazas 
			j=plazas; //Fuerzo a salir del bucle.
			idCamion++;		//id Para el siguiente camion
		}
	}
	escribirParking();
	pthread_mutex_unlock(&entradaSalidaParking);
	return NULL;






}

//Metodo que asigna un coche a una plaza
void *asignarPlaza(void *id){
	int j;
	pthread_mutex_lock(&entradaSalidaParking);
	if(plazasDisponibles <= 0){ //Si esta lleno el parking
		printf("Parking lleno, el coche esperará a entrar\n");
		pthread_cond_wait(&saleCoche,&entradaSalidaParking);
	}
	for (j=0; j<plazas; j++){ //Si no busco la primera plaza disponible
		if(parking[j]==0){  
			printf("ENTRADA: El coche %d aparca en %d. Plazas libres: %d\n",idCoche,j,--plazasDisponibles);
			parking[j] = idCoche; //La ocupo
			j=plazas; //Fuerzo a salir del bucle.
			idCoche++;		//id Para el siguiente coche
		}
	}
	escribirParking();
	pthread_mutex_unlock(&entradaSalidaParking);
	return NULL;
}

void salirParking(int posicion){
	int j;
	pthread_mutex_lock(&entradaSalidaParking);
	if (parking[posicion] != 0){  //Si la plaza aleatoria elegida esta ocupada
		if ( posicion == 0){
			//Si es la primera plaza y es un camion muestro mensaje y libero
			if ( parking[posicion + 1] == parking[posicion] ){ 
				plazasDisponibles += 2;
				printf("SALIDA: Camión %d saliendo. Plazas libres: %d\n",parking[posicion],plazasDisponibles);
				parking[posicion+1] =0;
				parking[posicion]=0;
			}
			else { //Si es un coche solo muestro y libero la primera plaza
				printf("SALIDA: Coche %d saliendo. Plazas libres: %d\n",parking[posicion],++plazasDisponibles);
				parking[posicion]=0;
			}
		}
		//Idem para ultima plaza
		else if ( posicion == plazas - 1 ){
			if ( parking[posicion] == parking[posicion-1]){
				plazasDisponibles += 2;
				printf("SALIDA: Camión %d saliendo. Plazas libres: %d\n",parking[posicion-1],plazasDisponibles);
				parking[posicion-1] =0;
				parking[posicion]=0;
			}
			else {
				printf("SALIDA: Coche %d saliendo. Plazas libres: %d\n",parking[posicion],++plazasDisponibles);
				parking[posicion]=0;
			}
		}
		//Si la plaza elegida aleatoriamente esta en medio
		else {
			if (parking[posicion + 1] == parking[posicion]){
				plazasDisponibles += 2;
				printf("SALIDA: Camión %d saliendo. Plazas libres: %d\n",parking[posicion],plazasDisponibles);
				parking[posicion+1] =0;
				parking[posicion]=0;
			} else if ( parking[posicion] == parking[posicion-1] ){
				plazasDisponibles += 2;
				printf("SALIDA: Camión %d saliendo. Plazas libres: %d\n",parking[posicion-1],plazasDisponibles);
				parking[posicion-1] =0;
				parking[posicion]=0;
			}
			else{
				printf("SALIDA: Coche %d saliendo. Plazas libres: %d\n",parking[posicion],++plazasDisponibles);
				parking[posicion]=0;
			}
		}		
	}
	//Si la plaza aleatoria elegida esta vacia coge la primera plaza ocupada y la vacia
	else {	
		for (j=0;j<plazas;j++){
			if (parking[j] != 0){
				if (j < plazas-1){ //Evitar errores al coger j+1 con parking
					if (parking[j] == parking [j+1]){ //Si es un camion
						plazasDisponibles += 2;
						printf("SALIDA: Camión %d saliendo. Plazas libres: %d\n",parking[j],plazasDisponibles);
						parking[j] =0;
						parking[j+1]=0;
						j=plazas;
					}
					else { //Si es un coche
						printf("SALIDA: Coche %d saliendo. Plazas libres: %d\n",parking[j],++plazasDisponibles);
						parking[j]=0;
						j=plazas;
					}
				}
				else { //Else de la ultima plaza (evitar errores j+1 con parking)
					printf("SALIDA: Coche %d saliendo. Plazas libres: %d\n",parking[j],++plazasDisponibles);
					parking[j]=0;
				}
			}
		}
	}
	pthread_cond_signal(&saleCoche);
	for (j=0; j<plazas-1; j++){
		if (parking[j]==0 && parking[j+1] == 0){ //Si al salir hay dos plazas seguidas llama a señal de dosHuecos para camiones.
			pthread_cond_signal(&dosHuecos);
		}
	}
	pthread_mutex_unlock(&entradaSalidaParking);
}
		
int main (int argc, char* argv[]){
	int i=0; // Variable para bucle
	int error,num_random,num_random2; //Error de thread al crearlo y numeros random para decidir si salir o entrar y si es coche o camion
	pthread_t coche[MAX_COCHES];  //Numero maximo de threads(coches) para entrar.
	srand(time(NULL));
	inicializarParking();
	inicializarEntradaSalida();
	while (i<MAX_COCHES){ //Bucle para programa.
		sleep(rand()%10); //Tiempo de espera
		num_random = rand() % plazas; //Le damos un valor entre 0 y el nº de plazas - 1
		if (plazas == plazasDisponibles){  //Si el parking está vacio obligatoriamente entra el coche que llegue.
			error = pthread_create(&coche[i],NULL,asignarPlaza,NULL);
			if (error){
				printf("Error al crear el thread"); 
				exit(-1);
			}
			i++;
		} else { //Si no entraran o saldran dado si num_random sea par o impar.
			if ((num_random % 2) == 0){// Si soy par
				num_random2= rand() %101; //Para saber si es un camion o un coche un nº aletorio entre 0 y 100
				if ((num_random2 % 5) == 0){ //Si es multiplo de 5 es un camion = 20% prob de que sea camion 80% coche
					error = pthread_create(&coche[i],NULL,asignarPlazaCamion,NULL);
					if (error){
						printf("Error al crear el thread");
						exit(-1);
					}
					i++;
				}
				else{ //Es coche
					error = pthread_create(&coche[i],NULL,asignarPlaza,NULL);
					if (error){
						printf("Error al crear el thread");
						exit(-1);
					}
					i++;
				}
			}
			else{ //Si no
				salirParking(num_random); //Llamada para sacar un coche del parking
			}
		}
	}
}

