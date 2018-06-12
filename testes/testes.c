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

void cortaComandoArgumento(char* str, char* argumento,char*comando)
{
	char* phash;
	char* pexc;
	
	argumento = NULL;
	comando = NULL;
	
	phash = strchr(str,'#');
	if(phash!=NULL)
	{
		str[phash-str] = '\0';
		argumento = phash+1;
	}
	pexc = strchr(argumento,'!');
	if(pexc!=NULL)
	{
		argumento[pexc-argumento]='\0';
		comando = str;
	}
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
	
	
	
	printf("argumento: %s \n",argumento);
	printf("comando: %s \n",comando);
}

int main (void)
{
	char str [256] = "seliga#99!";
	char comando[40];
	int argumento;
	
	while(!kbhit())
	{
		printf("helloworld!");
	}
	
	return 0;
}
