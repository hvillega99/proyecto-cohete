#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <string.h>

//Socket
int clientfd;

int open_clientfd(char *hostname, char *port);
void connection_error(int connfd);
void finalizar();

int main(int argc, char **argv)
{
	int *values;
	ssize_t n;

	//Direcciones y puertos
	char *hostname, *port;

	if(argc != 3){
		fprintf(stderr, "uso: %s <IP> <Puerto TCP>\n", argv[0]);
		return 1;
	}else{
		hostname = argv[1];
		port = argv[2];
	}

	//Valida el puerto
	int port_n = atoi(port);
	if(port_n <= 0 || port_n > USHRT_MAX){
		fprintf(stderr, "Puerto: %s invalido. Ingrese un número entre 1 y %d.\n", port, USHRT_MAX);
		fprintf(stderr, "uso: %s <IP> <Puerto>\n", argv[0]);
		return 1;
	}

	//Se conecta al servidor retornando un socket conectado
	clientfd = open_clientfd(hostname, port);

	if(clientfd < 0)
		connection_error(clientfd);

	printf("Conectado exitosamente a %s en el puerto %s.\nPresione ctrl + c para finalizar\n", hostname, port);

	signal(SIGINT, &finalizar);

	char alarma[32] = {0}, accion[50] = {0}, inclinacionX[52] = {0}, 
	inclinacionY[52] = {0}, distancia[16] = {0}, propulsor[16] = "Propulsor: on";
	int pointer[] = {0,0,0,0,0,0}, flag = 0;

	while(1){
	
		for(int i=0;i<6;i++){
			read(clientfd, &pointer[i], sizeof(int)); //Lee respuesta del servidor		
		}
		
		switch (pointer[4])
		{
		case 101:
			strcpy(alarma,"Fallo general");
			strcpy(accion,": Reiniciando propulsores");
			break;
		
		case 102:
			strcpy(alarma,"Fallo de motor principal");
			strcpy(accion,": Reiniciando propulsor principal");
			break;

		case 103:
			strcpy(alarma,"Fallo de motor de orientacion");
			strcpy(accion,": Reiniciando propulsores de orientacion");
			break;

		case 104:
			strcpy(alarma,"Autodestruccion manual");
			strcpy(accion,": Iniciando autodestruccion");
			flag = 2;
			break;

		default:
			strcpy(alarma,"Sin novedades");
			memset(accion,0,50);
		}

		switch (pointer[2])
		{
		case 0:
			strcpy(inclinacionX,"Propulsores: off;");
			break;
		
		default:
			strcpy(inclinacionX,"Propulsores: on;");
			break;
		}

		switch (pointer[3])
		{
		case 0:
			strcpy(inclinacionY,"Propulsores: off;");
			break;
		
		default:
			strcpy(inclinacionY,"Propulsores: on;");
			break;
		}

		switch (pointer[5])
		{
		case 0:
			strcpy(propulsor,"Propulsor: on");
			break;
		case 1:
			strcpy(propulsor,"Acelerando");
			break;
		case 2:
			strcpy(propulsor,"Desacelerando");
			break;
		}

		if(pointer[0]<100){
			strcpy(distancia,"Aterrizando");
		}
		
		if(pointer[0] == 1){
			strcpy(propulsor,"Propulsor: off");
			flag = 1;
		}

		printf("%s; Altura: %d m; %s|Combustible: %d%%|%s θ1: %d°|%s θ2: %d°|%s (%d)%s\n",
		propulsor,pointer[0],distancia,pointer[1],inclinacionX,pointer[2],inclinacionY,pointer[3],alarma,pointer[4],accion);
		if(flag != 0){break;}
		//if(pointer[0] == 1 || pointer[4] == 104){break;}
	}

	if(flag == 1){
		printf("\n¡Aterrizaje exitoso!\n");
	}else{
		while(1){
			for(int i=0;i<5;i++){
				read(clientfd, &pointer[i], sizeof(int)); //Lee respuesta del servidor		
			}
			printf("Autodestrucción en: %d\n", pointer[4]);
			if(pointer[4] == 1){
				sleep(1);
				break;
			}
		}
		printf("\n¡Cohete destruido!\n");
	}

	finalizar();
}

int open_clientfd(char *hostname, char *port) {
    int clientfd;
    struct addrinfo hints, *listp, *p;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  
    hints.ai_flags = AI_NUMERICSERV; 
    hints.ai_flags |= AI_ADDRCONFIG; 
    getaddrinfo(hostname, port, &hints, &listp);
  
    for (p = listp; p; p = p->ai_next) {

        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue; 
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) 
            break; 
        close(clientfd); 
    } 

    freeaddrinfo(listp);
    if (!p) 
        return -1;
    else   
        return clientfd;
}

void connection_error(int connfd)
{
	fprintf(stderr, "Error de conexión: %s\n", strerror(errno));
	close(connfd);
	exit(-1);
}

void finalizar(){
	close(clientfd);
	printf("\nDesconectando...\n");
	exit(0);
}