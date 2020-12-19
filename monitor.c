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

#define MAXLINE 1024

int open_clientfd(char *hostname, char *port);
void connection_error(int connfd);

int main(int argc, char **argv)
{
	int opt;

	//Socket
	int clientfd;
	//Direcciones y puertos
	char *hostname, *port;

	//Lectura desde consola
	char *linea_consola;
	char read_buffer[MAXLINE] = {0};
	size_t max = MAXLINE;
	ssize_t n, l = 0;
	
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

	printf("Conectado exitosamente a %s en el puerto %s.\n", hostname, port);


	linea_consola = (char *) calloc(1, MAXLINE);
	printf("Ingrese texto para enviar al servidor, escriba CHAO para terminar...\n");
	printf("> ");
	l = getline(&linea_consola, &max, stdin); //lee desde consola
	while(l > 0){
		n = write(clientfd, linea_consola, l); //Envia al servidor
		if(n<=0)
			break;

		n = read(clientfd, read_buffer, MAXLINE); //Lee respuesta del servidor
		if(n<=0)
			break;

		printf("%s", read_buffer);
		memset(read_buffer,0,MAXLINE); //Encerar el buffer

		//Volver a leer desde consola
		printf("> ");
		l = getline(&linea_consola, &max, stdin);
	}


	printf("Desconectando...\n");
	free(linea_consola);
	close(clientfd);

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