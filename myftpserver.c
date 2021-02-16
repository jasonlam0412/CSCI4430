#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 12345
int PORTno;

void messageAction(ftpMessage* receivedFromClient, int clientSocket){
	int messageLength = (int)sizeof(receivedFromClient->protocol) + (int)sizeof(receivedFromClient->type) + (int)sizeof(receivedFromClient->length);
	printf("%i\n", messageLength);
	if(strcmp(receivedFromClient->protocol, "myftp") != 0){
		char *errorMessage = "Wrong protocol to handle";
		printf("%s\n", errorMessage);
		send(clientSocket, errorMessage, strlen(errorMessage), 0);
	}
	else if(receivedFromClient->length != messageLength && (int)receivedFromClient->type == (int)0xA1){
		// Needs to change condition here since put_request and get_request have payload
		char *errorMessage = "Wrong message size";
		printf("%s\n", errorMessage);
		send(clientSocket, errorMessage, strlen(errorMessage), 0);
	}
	else{
		printf("Processing...\n");
		// LIST_REQUEST
		if((int)receivedFromClient->type == (int)0xA1){
			sendListReply(clientSocket);
		}
		// GET_REQUEST
		else if((int)receivedFromClient->type == (int)0xB1){
			sendGetReply(receivedFromClient, clientSocket);
		}
		// PUT_REQUEST
		else if((int)receivedFromClient->type == (int)0xC1){
			uploadFileFromClient(receivedFromClient, clientSocket);
		}
	}
}

void *connectionFunction(void *client_sd){
	char buffer[100];
	int length;
	int clientSocket = *((int *) client_sd);
	// handle the command function by recieving the command
	
	ftpMessage* receivedMessage;
	receivedMessage = receiveFTPMessage(clientSocket);
	//print out the message	(要改)
	printf("Received from sender: %s 0x%02X %d\n", receivedMessage->protocol, receivedMessage->type, receivedMessage->length);
	messageAction(receivedMessage, clientSocket);
	close(clientSocket);
	//pthread_exit(NULL);		-> no pthread status later add back 16/2/2021
}


int main(int argc, char** argv)
{
	//check only ./myftpserver PORT_NUMBER exist
	assert(argc == 2);
	PORTno = atoi(argv[1]);
	
	//socketBinding process start
	int sd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);
	socklen_t addrLen = sizeof(server_addr);
	if (bind(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		printf("bind error: %s (Errno:%d)\n", strerror(errno), errno);
		exit(0);
	}
	printf("Server in port: %d \n",PORTno);
	if (listen(sd, 3) < 0) {
		printf("listen error: %s (Errno:%d)\n", strerror(errno), errno);
		exit(0);
	}	
	
	//socketBinding process end
	

	while(1)
	{
		struct sockaddr_in client_addr;
		int addr_len = sizeof(client_addr);
		if ((client_sd = accept(sd, (struct sockaddr *)&client_addr, &addr_len)) <
			0) {
			printf("accept erro: %s (Errno:%d)\n", strerror(errno), errno);
			exit(0);
		} else {
			printf("receive connection from %s\n", inet_ntoa(client_addr.sin_addr));
		}
		
		connectfunction(&client_sd);
	}
	
}