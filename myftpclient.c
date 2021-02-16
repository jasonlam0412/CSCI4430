#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "myftp.h"

char IPADDR[15];
int PORT;
int sd;

int main(int argc, char** argv)	
{
	assert(argc >= 3 && argc <= 5);
    strcpy(IPADDR, argv[1]);
	PORT = atoi(argv[2]);
    
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(IPADDR);
	server_addr.sin_port = htons(PORT);
	socklen_t addrLen = sizeof(server_addr);
	/*
	if(connect(socketDescriptor, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
		printf("Connection Error: %s (Errno:%d)\n", strerror(errno), errno);
		exit(0);
	}
	*/
	if(strcmp(argv[3], "list") == 0)
	{}
	else if(strcmp(argv[3], "get") == 0)
	{}
	else if(strcmp(argv[3], "put") == 0)
	{}
}