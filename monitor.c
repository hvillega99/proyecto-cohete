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

//Socket
int clientfd;

int open_clientfd(char *hostname, char *port);
void connection_error(int connfd);
void catch();

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

	signal(SIGINT, &catch);

	int pointer[] = {0,0,0,0,0};
	while(1){
	
		for(int i=0;i<5;i++){
			read(clientfd, &pointer[i], sizeof(int)); //Lee respuesta del servidor
		}
		printf("distancia: %d; combustible: %d; giro1: %d; giro2: %d; alarma: %d\n",pointer[0],pointer[1],pointer[2],pointer[3],pointer[4]);
		
	}

	return 0;
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

void catch(){
	close(clientfd);
	printf("\nDesconectando...\n");
	exit(0);
}