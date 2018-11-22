#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "parser.h"
#define cd_cd "cd\0"



int main(int argc, char **argv) {
    char buf[1024];
    char* prompt= "msh> ";
	char* salida = "exit\n";
	FILE* fichero;
	tline* linea;
	int descriptor_error = dup(fileno(stderr));		//Guardamos descriptores para restaurar el valor alfinal de la linea, por si ha habido redirecciones.
	int descriptor_salida = dup(fileno(stdout));
	int descriptor_entrada = dup(fileno(stdin));
	signal(SIGINT,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);
	printf("%s",prompt);

	while ((fgets(buf,1024,stdin)!=NULL)&&(strcmp(buf,salida)!=0)){
		linea = tokenize(buf);
		
		if(linea == NULL){
			continue;	//Por si hay algun error
		}
		
		if (linea->ncommands==0){
        		printf(""); //Si no hay mandatos volvemos a poner el prompt			
		} 
		else {
	// ---------------------------------------------------- CD ----------------------------------------------------------------
			if (strcmp(linea->commands[0].argv[0],"cd\0") == 0){
				if (linea->ncommands > 1){
					printf("Uso: cd || cd ruta, solo 1 comando\n");
				}
				else {
					char* ruta_inicial = getenv("HOME");
					char ruta_completa[1024];
					if (linea->commands[0].argc == 1){					
						chdir(ruta_inicial);
						printf("%s\n",ruta_inicial);

					} 
					else if (linea->commands[0].argc == 2){
						char* ruta= linea->commands[0].argv[1];
							chdir(ruta);
							
							getcwd(ruta_completa,sizeof(ruta_completa));
							printf("%s\n",ruta_completa);
					} 
					else{
						printf("Uso: cd || cd ruta\n");
					}
				} //Fin cd bien hecho
			}	//Fin todo cd

	// ---------------------------------------------------- FIN CD ----------------------------------------------------------------
			
			pid_t* comandos= malloc(linea->ncommands*sizeof(int));
			int* tuberia= malloc((linea->ncommands-1)*sizeof(int)*2);
			int i;
							
			for (i=0; i < linea->ncommands;i++){
					if (i<linea->ncommands-1)
						pipe(&tuberia[2*i]);
					
					comandos[i]=fork();
						
					if(comandos[i]==0){		//HIJO
						signal(SIGINT,SIG_DFL);
						signal(SIGQUIT,SIG_DFL);
						if(linea-> ncommands>1){
							if (i<linea->ncommands-1){	//Si no soy el ultimo comando redirigimos la salida al pipe
								close(tuberia[2*i]);
								
								if (dup2(tuberia[2*i+1],1)<0){
									printf("error al redireccionar la salida con el descriptor numero %i\n",2*i+1);
									exit(1);
								}
								close(tuberia[2*i+1]);	
							}
							
						}
	//----------------------------------- REDIRECCIONES --------------------------------------------------
						
						if (linea->redirect_input!=NULL && i== 0){  //Fichero de entrada
							char* path=linea->redirect_input;
							fichero=fopen(path,"r");
							if (fichero==NULL){
								printf("Error en el fichero, no existe o esta corrupto\n");
								exit(1);
							}else{ //Si existe el fichero, cambia la entrada estandar a la entrada del fichero
								int entrada= fileno(fichero);
								dup2(entrada,fileno(stdin));
								fclose(fichero);		
							}
						} //Cierre input
						if (linea->redirect_output!=NULL && i == linea->ncommands - 1){ //Fichero de salida
							char* path=linea->redirect_output;
							fichero=fopen(path,"a");
							if (fichero==NULL){
								printf("Error en el fichero, no existe o esta corrupto\n");
								exit(1);
							}else{ //Si existe el fichero, cambia la salida estandar a la salida del fichero
								int salida= fileno(fichero);
								dup2(salida,fileno(stdout));
								fclose(fichero);
							}
						} //Cierre output
						if (linea->redirect_error!=NULL && i == linea->ncommands - 1){  //Fichero de error
							char* path=linea->redirect_error;
							fichero=fopen(path,"a");
							if (fichero==NULL){
								printf("Error en el fichero, no existe o esta corrupto\n");
								exit(1);
							}else{ //Si existe el fichero, cambia la salida de error estandar al fichero de error
								int error= fileno(fichero);
								dup2(error,fileno(stderr));
								fclose(fichero);
							}
						} //Cierre error
		//------------------------------------- FIN DE REDIRECCIONES -----------------------------------------------
					execvp(linea->commands[i].filename,linea->commands[i].argv); //Ejecuta instruccion
					printf("Algo salió mal");
					return 1;
					} 	//Fin hijo
					else{		//PADRE
						
						waitpid(comandos[i],NULL,0); //Espero a que termine el hijo
						
						if (linea->ncommands > 1){
							if (i<linea->ncommands-1){	//Si hay un comando despues ponemos a la entrada estandar la lectura del pipe.
								close(tuberia[2*i+1]);
								
								if(dup2(tuberia[2*i],0)<0){
									printf("error al redireccionar la entrada estandar con el descriptor numero %i\n",2*i);
									exit(1);
								}
								close(tuberia[2*i]);
							}
						}	
					} //Fin padre
			
			}	//Fin for
			free(comandos);
			free(tuberia);
			dup2(descriptor_entrada,0);  //Por si ha habido redirecciones restauramos sus valores.
			dup2(descriptor_salida,1);
			dup2(descriptor_error,2);
		}	//Fin else
		
		printf("%s",prompt);
		
	}	//Fin while
	return 0;

}
