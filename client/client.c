#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

#include "../graph/graph.h"
void* graph();
void* ctrl();
bool mensagem=false;
bool end=false;
bool retorno = false;
char buffer[256];


pthread_mutex_t mutexControle = PTHREAD_MUTEX_INITIALIZER;


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

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

char getInput()
{
	char line[20];
    char ch;

    if (fgets(line, sizeof line, stdin) == NULL) {
        printf("Input error.\n");
        exit(1);
    }

    ch = line[0];
    return ch;
}

int main(int argc, char *argv[])
{
	pthread_t ctrlthread,graphthread;
	int  iret1, iret2;
	iret1 = pthread_create( &ctrlthread, NULL, ctrl, NULL);
	if(iret1)
	{
	 fprintf(stderr,"Error ctrlthread - pthread_create() return code: %d\n",iret1);
	 exit(EXIT_FAILURE);
	}

	iret2 = pthread_create( &graphthread, NULL, graph,NULL);
	if(iret2)
	{
	 fprintf(stderr,"Error graphthread - pthread_create() return code: %d\n",iret2);
	 exit(EXIT_FAILURE);
	}
	
	
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	char tecla;
	
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    
    while(!end){
		portno = atoi(argv[2]);
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) 
			error("ERROR opening socket");
		server = gethostbyname(argv[1]);
		if (server == NULL) {
			fprintf(stderr,"ERROR, no such host\n");
			exit(0);
		}
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		bcopy((char *)server->h_addr, 
			 (char *)&serv_addr.sin_addr.s_addr,
			 server->h_length);
		serv_addr.sin_port = htons(portno);
		if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
				error("ERROR connecting");
		end = kbhit();
		while(!mensagem && !end);
		retorno = false;
		pthread_mutex_lock( &mutexControle );
		n = write(sockfd,buffer,strlen(buffer));
		pthread_mutex_unlock( &mutexControle );
		
		if (n < 0) 
			 error("ERROR writing to socket");
		mensagem = false;
		pthread_mutex_lock( &mutexControle );		
		bzero(buffer,256);
		n = read(sockfd,buffer,255);
		
		if (n < 0) 
			 error("ERROR reading from socket");
		printf("%s\n",buffer);
		retorno = true;
		pthread_mutex_unlock( &mutexControle );
		
		
		close(sockfd);
		
	}
    
	
    return 0;
}

void* graph( ) {
	/*Tdataholder *data;
	int j=0;
	int tmax=100;
	data = datainit(640,480,tmax,110,45,0,0);
	struct timespec graphTS;
	graphTS.tv_sec  = 0;
	graphTS.tv_nsec = 5*SIM_TS_DEFAULT*1000000L;
	while(!end)
	{
		pthread_mutex_lock( &mutexControle );
		datadraw(data,t/1000.0-j,pl.nivel,in.angleNow,out.angleNow);	
		pthread_mutex_unlock( &mutexControle );
		if(t/1000>tmax+j)
		{
			j+=tmax;

			data = datainit(640,480,tmax,110,pl.nivel,in.angleNow,out.angleNow);
		}
		nanosleep(&(graphTS), NULL);
			
	}
	while(1) {
		quitevent();
	}*/
	
	
}


void* ctrl() {
	Tdataholder *data;
	int j=0;
	int tmax=100;
	struct timespec graphTS;
	graphTS.tv_sec  = 0;
	graphTS.tv_nsec = 500*1000000L;
	int nivel;
	char state = 0;
	
	float error=0;
	float iError =0;
	float control =0;
	float ts=0.50;
	float Kp=10;
	float Ki=0;
	bool integrator = true;
	while(!end)
	{
		pthread_mutex_lock( &mutexControle );
		if(state ==0)
		{
			bzero(buffer,256);
			strcpy(buffer,"getNivel!");
			mensagem=true;
			state++;
		}else if(retorno)
		{
			char* pexc; 
			pexc= strchr(buffer,'!');
			if(pexc!=NULL)
			{
				*pexc = '\0';
			}
			nivel=atoi(buffer);
			if(state ==2)
			{
				if(nivel >= 100 || nivel <=0)
					integrator = false;
				else
					integrator=true;
				state =0;
			}
			else if(state==1)
			{
				printf("%s",buffer);
				
				printf("NIVEL ");
				bzero(buffer,256);
				error =50 - nivel; 
				if(integrator)
					iError += ts*error;
				control = Kp*error + Ki*iError;
				if(integrator)
				{
					if(control>=0)
						sprintf(buffer,"abreValvula#%d!",(int)control);
					else
						sprintf(buffer,"fechaValvula#%d!",(int)-control);
				}
				mensagem = true;
				state++;
			}
		}
		pthread_mutex_unlock( &mutexControle );
		nanosleep(&graphTS, NULL);
			
	}
	while(1) {
		quitevent();
	}
	
	
}


