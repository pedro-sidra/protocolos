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
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	char tecla;
	
    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    
    while(tecla!='n'){
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
		printf("Please enter the message: ");
		bzero(buffer,256);
		fgets(buffer,255,stdin);
		n = write(sockfd,buffer,strlen(buffer));
		if (n < 0) 
			 error("ERROR writing to socket");
		bzero(buffer,256);
		n = read(sockfd,buffer,255);
		if (n < 0) 
			 error("ERROR reading from socket");
		printf("%s\n",buffer);
		
		printf("Mandar mais uma?");
		tecla = getInput();
		close(sockfd);
		
	}
    
	
    return 0;
}
