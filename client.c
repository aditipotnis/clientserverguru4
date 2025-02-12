#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[1024];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(1);
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
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
	
    while (1){
	    bzero(buffer, 1024);
	    fgets(buffer, 1024, stdin);
	    n = write(sockfd, buffer, strlen(buffer));
	    if (n < 0) 
		    error("Error on writing");
	    printf("%s", buffer);

	    if (strncmp("> Comment: ", buffer, 11) == 0) {
	    	fgets(buffer, 1024, stdin);
		n = write(sockfd, buffer, strlen(buffer));
	    	if (n < 0)
		    error("Error on writing");
	    }
	    else {
		    fgets(buffer, 1024, stdin);
		    n = write(sockfd, buffer, strlen(buffer));
                    if (n < 0)
                    error("Error on writing");
	    }
	    if (strncmp("exit", buffer, 4) == 0){
		    break;
	    }
    }
}
