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
		*arg=-1000;
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
	char str [256] = "jeba";
	char comando[40];
	int argumento;
	
	getComandoArgumento(str,comando,&argumento);
	printf("argumento: %d \n",argumento);
	printf("comando: %s \n",comando);
	
	return 0;
}
