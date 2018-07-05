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
bool start=false;
bool end=false;
bool retorno = false;
char buffer[256];

double valvulaIn=0;
double nivel;
float erro=0;

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
    while(!start){
		
		printf("Insira comandos: ");
		bzero(buffer,256);
		fgets(buffer,255,stdin);
		if(strcmp("iniciaSimulacao!\n",buffer)==0)
		{
			start = true;
		}else if (strcmp(buffer,"end!")==0)
			end = true;
		n = write(sockfd,buffer,strlen(buffer));
		
		if (n < 0) 
			 error("ERROR writing to socket");
		bzero(buffer,256);
		n = read(sockfd,buffer,255);
		if (n < 0) 
			 error("ERROR reading from socket");
		printf("retorno:\n%s\n",buffer);
	
	}
    
    while(!end){
		end = kbhit();
		
		if(mensagem)
		{
			mensagem = false;
			
			// Envia mensagem
			pthread_mutex_lock( &mutexControle );
			n = write(sockfd,buffer,strlen(buffer));
			pthread_mutex_unlock( &mutexControle );
			
			if (n < 0) 
				 error("ERROR writing to socket");
			
			//Recebe retorno
			pthread_mutex_lock( &mutexControle );		
			bzero(buffer,256);
			n = read(sockfd,buffer,255);
			if (n < 0) 
				 error("ERROR reading from socket");
			retorno = true;
			pthread_mutex_unlock( &mutexControle );
		}
		
		
		
	}
	close(sockfd);
    
	
    return 0;
}
	float control =0;
void* graph( ) {
	Tdataholder *data;
	int j=0;
	double t = 0;
	int tmax=100;
	struct timespec graphTS;
	graphTS.tv_sec  = 0;
	int tGraph = 50;
	graphTS.tv_nsec = tGraph*1000000L;
	
	while(!start)
		if(end)
			return;
	data = datainit(640,480,tmax,110,45,0,0);
	while(!end)
	{
		t+=tGraph;
		pthread_mutex_lock( &mutexControle );
		datadraw(data,t/1000.0-j,nivel,valvulaIn,abs(control));	
		pthread_mutex_unlock( &mutexControle );
		if(t/1000>tmax+j)
		{
			j+=tmax;

			data = datainit(640,480,tmax,110,nivel,valvulaIn,abs(control));
		}
		nanosleep(&(graphTS), NULL);
			
	}
	while(1) {
		quitevent();
	}
	
	
}



void* ctrl() {
	Tdataholder *data;
	int j=0;
	int tmax=100;
	struct timespec graphTS;
	graphTS.tv_sec  = 0;
	int tcontrol = 10;
	graphTS.tv_nsec = tcontrol*1000000L;
	int numReturn;
	int state = 0;

	float iError =0;
	float dError = 0;
	float ts=tcontrol*0.001;
	//float Kp=10;
	float Kp=.5;//0.5

	float Ki=0;
	//float Kd = 10;
	float Kd = 0.05;//0.05

	float erroLast;
	bool integrator = true;
	int valvulaPlanta=0;
	while(!start)
		if(end)
			return;
	while(!end)
	{
			pthread_mutex_lock( &mutexControle );
			
			if(state ==-1)
			{
				bzero(buffer,256);
				strcpy(buffer,"iniciaSimulacao!");
				mensagem=true;
				state++;
			}else if(state ==0)
			{
				bzero(buffer,256);
				strcpy(buffer,"getNivel!");
				mensagem=true;
				state++;
			}else if(retorno)
			{
				retorno = false;
				char* pexc; 
				pexc= strchr(buffer,'!');
				if(pexc!=NULL)
				{
					*pexc = '\0';
				}
				numReturn=atoi(buffer);
				if(state ==2)
				{
					//printf("Valvula: %d\n",numReturn);
					state =0;
				}
				else if(state==1)
				{
					nivel = numReturn;
					bzero(buffer,256);
					erroLast = erro;
					erro =(double)(50 - nivel); 
					if(integrator)
						iError += ts*erro;
					dError = (erro-erroLast) / ts;
					control = Kp*erro + Ki*iError + Kd*dError;
					//printf("I: %f",iError);
					if(valvulaIn+control >= 100)
					{
						control = 100-valvulaIn;
						integrator = false;
					}else if(valvulaIn+control <=0){
						control = 0-valvulaIn;
						integrator = false;
					}
					else
						integrator=true;
					valvulaIn += control;
					int diffValvula = fabs(valvulaIn-valvulaPlanta);
					if(diffValvula!=0)
					{
						if(control>=0)
						{
							sprintf(buffer,"abreValvula#%d!",diffValvula);
							valvulaPlanta+=diffValvula;
						}
						else
						{
							sprintf(buffer,"fechaValvula#%d!",diffValvula);
							valvulaPlanta-=diffValvula;
						}
					}else
						sprintf(buffer,"fechaValvula#%d!",0);
					
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


