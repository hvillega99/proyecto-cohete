#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>

#define SHMSZ     4
#define NUM     5
#define SHM_ADDR  233

int *param[NUM], *distancia, *nivel, *giro1, *giro2, *alarma;
int shmid[NUM], values[NUM]; 

int propulsor = 0, propulsorX = 0, propulsorY = 0;

sem_t mtpx, mtpy, flags[NUM];

pthread_t tidDistancia, tidCombustible, tidPropulsorX, tidPropulsorY, tidAlarma;

int inicializar_memoria_compartida(void);
int open_listenfd(char *port);
void connection_error(int connfd);
void* thread_monitor(void* arg);
void atender_cliente(int connfd);
void* controlDistancia();
void* controlCombustible();
void* controlPropulsorX();
void* controlPropulsorY();
void* controlAlarma();
void terminar();

int main(int argc, char **argv){
    
	//Sockets
	int listenfd;
	unsigned int clientlen;
	//Direcciones y puertos
	struct sockaddr_in clientaddr;
	struct hostent *hp;
	char *haddrp, *port;

	if(argc != 2){
		fprintf(stderr, "uso: %s <Puerto TCP>\n", argv[0]);
		return 1;
	}else
		port = argv[1];

	//Valida el puerto
	int port_n = atoi(port);
	if(port_n <= 0 || port_n > USHRT_MAX){
		fprintf(stderr, "Puerto: %s invalido. Ingrese un número entre 1 y %d.\n", port, USHRT_MAX);
		return 1;
	}

    //Accede a la memoria compartida
    if (inicializar_memoria_compartida()==-1) return 1;

	//Abre un socket de escucha en port
	listenfd = open_listenfd(port);

	if(listenfd < 0)
		connection_error(listenfd);

    sem_init(&mtpx,0,1);
    sem_init(&mtpy,0,1);

    for(int i=0;i<5;i++){
        sem_init(&flags[i],0,1);
    }

    pthread_create(&tidDistancia, NULL, &controlDistancia, NULL);
    pthread_create(&tidCombustible, NULL, &controlCombustible, NULL);
    pthread_create(&tidPropulsorX, NULL, &controlPropulsorX, NULL);
    pthread_create(&tidPropulsorY, NULL, &controlPropulsorY, NULL);
    pthread_create(&tidAlarma, NULL, &controlAlarma, NULL);

    signal(SIGINT, &terminar);

	printf("Escuchando en el puerto %s...\nPresione ctrl + c para finalizar\n\n", port);

	while (1) {
		clientlen = sizeof(clientaddr);
		int connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);

		/* Determine the domain name and IP address of the client */
		hp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
					sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		haddrp = inet_ntoa(clientaddr.sin_addr);

		printf("Se ha conectado el cliente %s (%s)\n", hp->h_name, haddrp);
		
        pthread_t tidCliente;
        pthread_create(&tidCliente, NULL, &thread_monitor, (void*)&connfd);
	}
}

//*******Métodos**********************


int open_listenfd(char *port) 
{
    struct addrinfo hints, *listp, *p;
    int listenfd, optval=1;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  /* Accept TCP connections */
    hints.ai_flags = AI_PASSIVE;      /* ... on any IP address */
    hints.ai_flags |= AI_NUMERICSERV; /* ... using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG;  /* Recommended for connections */
    getaddrinfo(NULL, port, &hints, &listp);

    /* Walk the list for one that we can bind to */
    for (p = listp; p; p = p->ai_next) {

        /* Create a socket descriptor */
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue;  /* Socket failed, try the next */

        /* Eliminates "Address already in use" error from bind */
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
                   (const void *)&optval , sizeof(int));

        /* Bind the descriptor to the address */
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break; /* Success */
        close(listenfd); /* Bind failed, try the next */
    }

    /* Clean up */
    freeaddrinfo(listp);
    if (!p) /* No address worked */
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, 1024) < 0)
		return -1;
    return listenfd;
}

void* thread_monitor(void* arg){
    int* connfd = (int*)arg;
    atender_cliente(*connfd);
    printf("Un cliente se ha desconectado...\n");
    close(*connfd);
}

void atender_cliente(int connfd){	

	int n;
   
    while(1){ //escribo datos en el socket
       
        for(int i=0;i<5;i++){

            sem_wait(&flags[i]);
            n = write(connfd, &values[i], sizeof(int));
            sem_post(&flags[i]);

            if(n <= 0)
                break;
        }  

        if(n <= 0){
            return;
        }
    }
}

void connection_error(int connfd)
{
	fprintf(stderr, "Error de conexión: %s\n", strerror(errno));
	close(connfd);
	exit(-1);
}

int inicializar_memoria_compartida(void){
    int i;

    for (i=0;i<NUM;i++){
        if ((shmid[i] = shmget(SHM_ADDR+i, SHMSZ, 0666)) < 0) { 
            perror("shmget");
            return(-1);
        }
        if ((param[i] = shmat(shmid[i], NULL, 0)) == (int *) -1) {
            perror("shmat");
            return(-1);
        }
    }
  
    distancia=param[0];
    nivel=param[1];
    giro1=param[2];
    giro2=param[3];
    alarma=param[4];

    return(1);
}

void* controlDistancia(){
    
    int h = 0;
    while(1){
        
        h = *distancia;

        sem_wait(&flags[0]);
        values[0] = h;
        sem_post(&flags[0]);
    }
}

void* controlCombustible(){

    while(1){
        sem_wait(&flags[1]);
        values[1] = *nivel;  
        sem_post(&flags[1]);      
    }
    
}

void* controlPropulsorX(){
    int inclinacion = 0;

    while(1){

        inclinacion = *giro1;

        if(inclinacion != 0){
            sem_wait(&mtpx);
            propulsorX = 1;
            sem_post(&mtpx);
            sleep(2);

            if(inclinacion < 0){
                inclinacion++;
            }else if(inclinacion > 0){
                inclinacion--;
            }
            *giro1 = inclinacion;

        }else{
            if(propulsorX == 1){
                sem_wait(&mtpx);
                propulsorX = 0;
                sem_post(&mtpx);
            }
        }
        sem_wait(&flags[2]);
        values[2] = inclinacion;
        sem_post(&flags[2]);
    }
}

void* controlPropulsorY(){

    int inclinacion = 0;

    while(1){

        inclinacion = *giro2;

        if(inclinacion != 0){
            sem_wait(&mtpy);
            propulsorY = 1;
            sem_post(&mtpy);

            sleep(2);

            if(inclinacion < 0){
                inclinacion++;
            }else if(inclinacion > 0){
                inclinacion--;
            }

            *giro2 = inclinacion;
            
        }else{
            if(propulsorY == 1){
                sem_wait(&mtpy);
                propulsorY = 0;
                sem_post(&mtpy);
            }
        }
        sem_wait(&flags[3]);
        values[3] = inclinacion;
        sem_post(&flags[3]);
    }

}

void* controlAlarma(){

    while(1){
        sem_wait(&flags[4]);
        values[4] = *alarma;
        sem_post(&flags[4]);
    }
    
}

void terminar(){
	printf("\nFinalizando...\n");
    for (int i=0;i<NUM;i++)
	    close(shmid[i]);
	exit(0);
}