// C program rocket simulation
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include <pthread.h>

#define SHMSZ     4
#define NUM     5
#define SHM_ADDR  233

int *param[NUM], *distancia, *nivel, *giro1, *giro2, *alarma;
int intervalo,shmid[NUM];
pthread_t tid; 

int inicializar_memoria_compartida(void);
void sig_handlerINT(int signo);
void *descender(void * param);
  
int main(int argc, char *argv[])  
{   
    pthread_attr_t attr; 

    if (signal(SIGINT, sig_handlerINT) == SIG_ERR)
	printf("\ncan't catch SIGINT\n");
     
    if (argc != 4)  
    { 
        printf("enter 1 arguments only eg.\"filename distancia porcentaje_combustible intervalo_descenso !\""); 
        return 0; 
    } 
  
   if (inicializar_memoria_compartida()==-1) exit(-1);
   *distancia=atoi(argv[1]);  
   *nivel = atoi(argv[2]);  
   intervalo = atoi(argv[3]);  

   *giro1 =0;
   *giro2 =0;
   *alarma= 0;
   
    pthread_attr_init(&attr);
    pthread_create(&tid,&attr,descender,NULL);
   

   while(1){
	printf("Valor actual giro1 giro2 alarma= %d %d %d \n",*giro1,*giro2,*alarma);
        scanf("%d %d %d",giro1, giro2, alarma);
	printf("Nuevo valor giro1 giro2 alarma= %d %d %d \n",*giro1,*param[3],*param[4]);	
	printf("Valor actual distancia %d, combustible %d\n",*distancia,*nivel);
   }
}

int inicializar_memoria_compartida(void){
int i;

 for (i=0;i<NUM;i++){
    if ((shmid[i] = shmget(SHM_ADDR+i, SHMSZ, IPC_CREAT | 0666)) < 0) { 
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

void sig_handlerINT(int signo)
{ int i;
  if (signo == SIGINT){
    printf("Falla castratrofica de cohete");
    *alarma=101;
    sleep(1);
    for (i=0;i<NUM;i++)
	close(shmid[i]);
  }
   pthread_cancel(tid);
   exit(1);
return;
}

void *descender(void * param)
{

  while(1){

   sleep(intervalo);
   *distancia=*distancia - 1; //desciende 1 metro por intervalo
   *nivel=*nivel - 1;

    if (*distancia < 0){
	*alarma=101; //cohere llegó al suelo
         pthread_exit(0);
    }
  } 
}

