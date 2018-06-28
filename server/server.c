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
#include <time.h>
#include <math.h>
#include <pthread.h>

#include "../graph/graph.h"

#define DEBUG true
#define SIM_TS_DEFAULT 10

int getComandoArgumento(char*str, char*cmd,int* arg);
int kbhit(void);
void error(const char *msg);
void handleMensagem(char* msg,char* retorno);
void *comms();
void *simPlanta();
void initPlanta();
double saturate(double val, double upper, double lower);
void*graph(void*);

struct planta{
	double nivel;
	struct timespec simTS;
	double comandoValvula;
	double MAX;
};
struct valvula {
	double angleNow;
	double angleNext;
};

struct level{
	double now;
	double next;
};
pthread_mutex_t mutexPlanta = PTHREAD_MUTEX_INITIALIZER;

int argc =0;
char **argv;
bool end = false;
double t=0;

struct planta pl;
struct level lv;
struct valvula in;
struct valvula out;

int main(int argcl,char *argvl[])
{
	argc = argcl;
	argv = argvl;
	
	initPlanta();
	
	pthread_t commthread, simthread,graphthread;
	int  iret1, iret2,iret3;
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
	
	iret3 = pthread_create( &graphthread, NULL, graph,NULL);
	if(iret3)
	{
	 fprintf(stderr,"Error graphthread - pthread_create() return code: %d\n",iret3);
	 exit(EXIT_FAILURE);
	}
	
	while(!kbhit());
	end = true;
	pthread_join( commthread, NULL);
	pthread_join( simthread, NULL); 

	exit(EXIT_SUCCESS);
	return 0;
}

// PLANTA:
double outangle(double t) {
	if(t <= 0) {
		return 50;
	}
	if(t<20000) {
		return (50+t/400);
	}
	if(t<30000) {
		return 100;
	}
	if(t<50000) {
		return (100-(t-30000)/250);
	}
	if(t<70000) {
		return (20 + (t-50000)/1000);
	}

	if(t<100000) {
		return(40+20*cos((t-70000)*2*M_PI/10000));
	}
	return 100;
}

void initPlanta()
{
	pl.nivel = 0.4;
	pl.MAX=100;
	pl.comandoValvula=0;
	pl.simTS.tv_sec  = 0;
	pl.simTS.tv_nsec = SIM_TS_DEFAULT*1000000L;
	lv.now = 0.4;
	lv.next = 0.4;
	in.angleNow = 0;
	in.angleNext = 0;
	out.angleNow = 0;
	out.angleNext = 0;	
}

double saturate(double val, double lower, double upper)
{
	if(val > upper)
		return upper;
	else if(val < lower)
		return lower;
	return val;
}
void *simPlanta()
{
	double dT = SIM_TS_DEFAULT;
	double delta = 0;
	double influx, outflux;
	
	

	while(!end)
	{
		pthread_mutex_lock( &mutexPlanta );
		out.angleNow = saturate(out.angleNext,0,100);
		in.angleNow = saturate(in.angleNext,0,100);
		lv.now = saturate(lv.next,0,1);
		pl.nivel=lv.now*100;
		dT = (double)pl.simTS.tv_nsec/1000000.0;
		t+=dT;
		out.angleNext = outangle(t);
		//Rotina de simulacao:
		if (pl.comandoValvula!=0) {
			delta   += (pl.comandoValvula);
			pl.comandoValvula = 0;
		}
		if (delta > 0) {
			if(delta < 0.02*dT) 
			{
				in.angleNext=  in.angleNow+delta;
				delta = 0; 
			}else
			{
				in.angleNext=  in.angleNow+0.02*dT;
				delta -=  0.02*dT;
			}
		} else if (delta < 0) {  
			if(delta > -0.02*dT) {
				in.angleNext= in.angleNow+delta;
				delta = 0;
			} 
			else {
				in.angleNext=  in.angleNow-0.02*dT;
				delta +=  0.02*dT;
			}
		}

		influx = 1*sin(M_PI/2*in.angleNow/100);
		outflux= (pl.MAX/100.0)*(lv.now/1.25+0.2)*sin(M_PI/2*out.angleNow/100);
		lv.next=lv.now+0.00001*dT*(influx-outflux);
		
		//Fim da rotina de simulacao
		pthread_mutex_unlock( &mutexPlanta );
		nanosleep(&pl.simTS, NULL);
	}
}
// COMUNICAÇÃO:
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
     listen(sockfd,5);
	 clilen = sizeof(cli_addr);
	 newsockfd = accept(sockfd, 
				 (struct sockaddr *) &cli_addr, 
				 &clilen);
	 if (newsockfd < 0) 
		  error("ERROR on accept");
     while(!end)
     {
		 strcpy(retorno,"No Commands");
		 
		 bzero(buffer,256);
		 n = read(newsockfd,buffer,255);
		 if (n < 0) error("ERROR reading from socket");
		 
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
	if(DEBUG)
	{
		printf("argumento: %d \n",argumento);
		printf("comando: %s \n",comando);
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		printf("now: %d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	}
	pthread_mutex_lock( &mutexPlanta );
	
	// ABRE VALVULA
	if(!strcmp("abreValvula",comando))
	{
		pl.comandoValvula = saturate((double)argumento,0.0,100.0);
		sprintf(retorno,"%d!",(int) saturate(in.angleNow+argumento,0,100));	
		
	}
	// FECHA VALVULA 
	else if(!strcmp("fechaValvula",comando))
	{
		pl.comandoValvula = -saturate(argumento,0,100);
		sprintf(retorno,"%d!",(int) saturate(in.angleNow-argumento,0,100));
	}
	// GET NIVEL 
	else if(!strcmp("getNivel",comando))
	{
		argumento = (int)pl.nivel;
		sprintf(retorno,"%d!",argumento);
	}
	// TESTA CONEXAO
	else if(!strcmp("testaConexao",comando))
	{
		strcpy(retorno,"OK!");
	}
	// SET PERIODO SIMULACAO
	else if(!strcmp("setPeriodoSimulacao",comando))
	{
		long tvnew = (long)argumento*1000L*1000L;
		if(tvnew < 999999999L && tvnew>0)
		{
			pl.simTS.tv_nsec=tvnew;
			sprintf(retorno,"%d!",argumento);
		}else
		{
			sprintf(retorno,"NAO!");
		}
	}
	// SET CONSUMO
	else if(!strcmp("setConsumo",comando))
	{
		pl.MAX = saturate(argumento,0,100);
		sprintf(retorno,"%d!",argumento);
	}
	// INICIA SIMULACAO
	else if(!strcmp("iniciaSimulacao",comando))
	{
		strcpy(retorno,"OK!");
	}
	pthread_mutex_unlock( &mutexPlanta );
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


// GRAPH:


void* graph( void* argIn ) {
	Tdataholder *data;
	int j=0;
	int tmax=100;
	data = datainit(640,480,tmax,110,45,0,0);
	struct timespec graphTS;
	graphTS.tv_sec  = 0;
	graphTS.tv_nsec = 5*SIM_TS_DEFAULT*1000000L;
	while(!end)
	{
		pthread_mutex_lock( &mutexPlanta );
		datadraw(data,t/1000.0-j,pl.nivel,in.angleNow,out.angleNow);	
		pthread_mutex_unlock( &mutexPlanta );
		if(t/1000>tmax+j)
		{
			j+=tmax;

			data = datainit(640,480,tmax,110,pl.nivel,in.angleNow,out.angleNow);
		}
		nanosleep(&(graphTS), NULL);
			
	}
	while(1) {
		quitevent();
	}
	
	
}

// MISC:

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



