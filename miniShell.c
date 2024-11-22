#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include "parser.h"


void manejador();
int ejecutar();
int i,in,out,err,nc,bg;
tcommand * coms;
int **pipes;
pid_t * hijosActual;

int main(int argc, char * argv) {
	if (argc > 1){
		printf("El ejecutable %d no necesita argumentos\n",argv[0]);
		return 1;
	}
	char buf[1024];
	tline * line;

	printf("msh> ");	
	while (1) {
		if (fgets(buf, 1024, stdin) == NULL){
			if (feof(stdin)){
				printf("\n");
				break;
			} 
			fprintf(stderr, "Error: Fallo al leer la entrada: %s\n", strerror(errno));
		}
		in = 0;
		out = 0;
		err = 0;
		nc = 0;
		bg = 0;
		line = tokenize(buf);
		if (line==NULL) {
			printf("Error al leer la linea\nmsh> ");
			continue;
		}
		if (line->redirect_input != NULL) {
			in = 1;
		}
		if (line->redirect_output != NULL) {
			out = 1;
		}
		if (line->redirect_error != NULL) {
			err = 1;
		}
		nc = line->ncommands;
		bg = line->background;
		coms = line->commands;
		printf("nc = %d\n",nc);//BORRAR
		ejecutar();
		printf("msh> ");	
	}
	return 0;
}

int ejecutar(){
	hijosActual = (pid_t *)malloc(nc * sizeof(pid_t)); //Reservar nc espacios para los nc hijos
	int j,k;
	for(j = 0; j < nc; j++){
		hijosActual[j] = 0;
	}
	pipes = (int **)malloc((nc - 1) * sizeof(int *));
	for(j = 0; j < nc - 1; j++){
		pipes[j] = (int *)malloc(2 * sizeof(int));
		pipe(pipes[j]);
	}
	signal(SIGUSR1, manejador);
	pid_t pid;
	for (i = 0; i < nc; i++){
		k = nc - 1 - i;
		pid = fork();
		if (pid < 0){ //Error en el fork
			fprintf(stderr, "Falló el fort()\n%s\n", strerror(errno));
			exit(-1);
		} else if (pid == 0){ //Código hijo i
			if (i == nc - 1){ //1º Hijo a ejecutar
				printf("i = %d\n",i);//BORRAR
				if (nc > 1){
					close(pipes[nc - 2][0]);
					dup2(pipes[nc - 2][1],STDOUT_FILENO);
					close(pipes[nc - 2][1]);
				}
				pause();
			} else if (i == 0){//Último Hijo a ejecutar
				printf("i = %d\n",i);//BORRAR
				close(pipes[0][1]);
				dup2(pipes[0][0],STDIN_FILENO);
				close(pipes[0][0]);
				for (j = 1; j < nc - 1;j++){//Cierro todos los demás pipes
					close(pipes[j][0]);
					close(pipes[j][1]);
				}
			} else { //Los hijos del medio
				printf("i = %d\n",i);//BORRAR
				close(pipes[i][0]);
				close(pipes[i - 1][1]);
				dup2(pipes[i][1],STDOUT_FILENO);
				dup2(pipes[i - 1][0],STDIN_FILENO);
				close(pipes[i][1]);
				close(pipes[i - 1][0]);
				for (j = i + 1; j < nc - 1;j++){//Cierro todos los demás pipes
					close(pipes[j][0]);
					close(pipes[j][1]);
				}
			}
			execvp(coms[k].filename,coms[k].argv);
			perror("Error en execvp");
    			exit(1);
		} else {
			hijosActual[i] = pid;
			if (i > 0){
				close(pipes[i - 1][0]);
				close(pipes[i - 1][1]);
			}
			
		}
	}
	kill(hijosActual[nc - 1], SIGUSR1);//Te aseguras que el último hijo sea último en ejecutar
	for (j = 0; j < nc; j++){
		wait(NULL);
	}
	free(hijosActual);
	for(j = 0; j < nc - 1; j++){
		free(pipes[j]);
	}
	free(pipes);
	return 0;

}


void manejador(){
	execvp(coms[nc - 1].filename,coms[nc - 1].argv);
	perror("Error en execvp");
    	exit(1);
}

