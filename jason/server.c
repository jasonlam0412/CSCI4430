#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <signal.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#include "myftp.h"

#define PORT 12345
Message* list_reply(int);
Message* get_reply(bool);
Message* put_reply();
void *connectionFunction(void *);


Message* list_reply(int payloadLength){
    Message *listReplyMessage = (Message *) malloc(sizeof(Message));
    strcpy(listReplyMessage->protocol, "myftp");
    listReplyMessage->type = 0xA2;
    listReplyMessage->length = 11 + payloadLength;
    return listReplyMessage;
}

Message* get_reply(bool existance){
    Message *getReplyMessage = (Message *) malloc(sizeof(Message));
    if(existance)
        getReplyMessage->type = 0xB2;
    else
        getReplyMessage->type = 0xB3;
    strcpy(getReplyMessage->protocol, "myftp");
    getReplyMessage->length = 11;
    return getReplyMessage;
}

Message* put_reply(){
    Message *putReplyMessage = (Message *) malloc(sizeof(Message));
    strcpy(putReplyMessage->protocol, "myftp");
    putReplyMessage->type = 0xC2;
    putReplyMessage->length = 11;
    return putReplyMessage;
}



/*
 * Worker thread performing receiving and outputing messages
 */


void *connectionFunction(void *client_sd){
	char buffer[100];
	int length;
	int clientSocket = *((int *) client_sd);
	// printf("RECEIVING SOMETHING\n");
	Message* receivedMessage;
	// Client is now connected to server
	// recv(clientSocket, (void *) receivedMessage, sizeof(receivedMessage), 0);
	receivedMessage = receiveMessage(clientSocket);
	printf("Received from sender: %s 0x%02X %d\n", receivedMessage->protocol, receivedMessage->type, receivedMessage->length);
	//messageAction(receivedMessage, clientSocket);
	close(clientSocket);
	pthread_exit(NULL);
}

int main(int argc, char **argv) {
	int threadNumber = 0;
	pthread_t *threadID;
	threadID = (pthread_t *) malloc(sizeof(pthread_t));
	int i = 0;
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1){
		printf("Socket descriptor creation error.\n");
		exit(0);
	}
	int client_sd;
	assert(argc == 2);
	int port_num = atoi(argv[1]);

	//socket bind and listen//////////
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port_num);
	if (bind(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
	printf("bind error: %s (Errno:%d)\n", strerror(errno), errno);
	exit(0);
	}
	if (listen(sd, 3) < 0) {
	printf("listen error: %s (Errno:%d)\n", strerror(errno), errno);
	exit(0);
	}
	////socket bind and listen end//////////////

	struct sockaddr_in client_addr;
	int addr_len = sizeof(client_addr);
	while(1){
		if ((client_sd = accept(sd, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
		printf("accept erro: %s (Errno:%d)\n", strerror(errno), errno);
		exit(0);
		} else {
		printf("receive connection from %s\n", inet_ntoa(client_addr.sin_addr));
		}
		pthread_create(&threadID[threadNumber], NULL, connectionFunction, &client_sd);
		threadNumber += 1;
		pthread_t *newPointer = (pthread_t *) realloc(threadID, sizeof(pthread_t) * (threadNumber + 1));
		if(newPointer != NULL)
			threadID = newPointer;

		
	}
	for(int i=0; i<threadNumber; i++)
			pthread_join(threadID[i], NULL);
		
	free(threadID);

	close(sd);
	return 0;
}
