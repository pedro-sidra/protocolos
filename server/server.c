/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

#include <pthread.h>

int getComandoArgumento(char*str, char*cmd,int* arg);
int kbhit(void);
void error(const char *msg);
void handleMensagem(char* msg,char* retorno);
void *comms();
void *simPlanta();
void initPlanta();

struct tanque{
	double nivel;
	double influx;
	double outflux;
};

pthread_mutex_t mutexPlanta = PTHREAD_MUTEX_INITIALIZER;

int argc =0;
char **argv;
bool end = false;
struct tanque planta;
double simTS = 1;

int main(int argcl,char *argvl[])
{
	argc = argcl;
	argv = argvl;
	
	initPlanta();
	
	pthread_t commthread, simthread;
	int  iret1, iret2;
	iret1 = pthread_create( &commthread, NULL, comms,NULL);
	if(iret1)
	{
	 fprintf(stderr,"Error commthread - pthread_create() return code: %d\n",iret1);
	 exit(EXIT_FAILURE);
	}

	iret2 = pthread_create( &simthread, NULL, simPlanta,NULL);
	if(iret2)
	{
	 fprintf(stderr,"Error simthread - pthread_create() return code: %d\n",iret2);
	 exit(EXIT_FAILURE);
	}
	
	while(!kbhit());
	end = true;

	pthread_join( commthread, NULL);
	pthread_join( simthread, NULL); 

	exit(EXIT_SUCCESS);
	return 0;
}
void initPlanta()
{
	planta.nivel = 0.4;
	planta.influx = 0;
	planta.outflux=0;
}
void *simPlanta()
{
	int i;
	while(!end)
	{
		pthread_mutex_lock( &mutexPlanta );
		planta.nivel+=0.01;
		pthread_mutex_unlock( &mutexPlanta );
		sleep(simTS);
	}
}
void *comms()
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     char retorno[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
    
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
              
     while(!end)
     {
		 strcpy(retorno,"No Commands");
		 listen(sockfd,5);
		 clilen = sizeof(cli_addr);
		 newsockfd = accept(sockfd, 
					 (struct sockaddr *) &cli_addr, 
					 &clilen);
		 if (newsockfd < 0) 
			  error("ERROR on accept");
		 bzero(buffer,256);
		 n = read(newsockfd,buffer,255);
		 if (n < 0) error("ERROR reading from socket");
		 printf("Here is the message: %s\n",buffer);
		 
		 handleMensagem(buffer,retorno);
		 
		 n = write(newsockfd,retorno,strlen(retorno));
		 if (n < 0) error("ERROR writing to socket");	 
	 }
     close(newsockfd);
     close(sockfd);
}

void handleMensagem(char* msg, char* retorno)
{
	char comando[40];
	int argumento;
	
	getComandoArgumento(msg,comando,&argumento);
	printf("argumento: %d \n",argumento);
	printf("comando: %s \n",comando);
	
	pthread_mutex_lock( &mutexPlanta );
	if(!strcmp("abreValvula",comando))
	{
		sprintf(retorno,"Angulo da valvula: %d",argumento);	
	}else if(!strcmp("fechaValvula",comando))
	{
		sprintf(retorno,"Angulo da valvula: %d",argumento);
	}else if(!strcmp("getNivel",comando))
	{
		argumento = (int) 100*planta.nivel;
		sprintf(retorno,"Nivel do tanque: %d",argumento);
	}else if(!strcmp("testaConexao",comando))
	{
		strcpy(retorno,"OK");
	}else if(!strcmp("setPeriodoSimulacao",comando))
	{
		simTS=argumento/1000;
		sprintf(retorno,"Periodo de sim setado: %d ms",argumento);
	}else if(!strcmp("setConsumo",comando))
	{
		sprintf(retorno,"Consumo setado: %d",argumento);
	}else if(!strcmp("iniciaSimulacao",comando))
	{
		strcpy(retorno,"OK");
	}
	pthread_mutex_unlock( &mutexPlanta );
}

int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;
 
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
 
  ch = getchar();
 
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
 
  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }
 
  return 0;
}

int getComandoArgumento(char*str, char*cmd,int* arg)
{
	char* phash;
	char* pexc;
	
	char* argumento;
	char* comando;
	argumento = NULL;
	comando = NULL;
	
	phash = strchr(str,'#');
	if(phash!=NULL)
	{
		*phash = '\0';
		argumento = phash+1;
	}else
	{
		argumento = str;
	}
	pexc = strchr(argumento,'!');
	if(pexc!=NULL)
	{
		*pexc='\0';
		comando = str;
	}
	if(phash==NULL)
		argumento = NULL;
	
	if(argumento!=NULL)
	{
		*arg =  atoi(argumento);
	}else
	{
		*arg=-1;
	}
	if(comando!=NULL)
	{
		strcpy(cmd,comando);
	}else
	{
		strcpy(cmd, "NONE");
	}
}


void error(const char *msg)
{
    perror(msg);
    exit(1);
}


