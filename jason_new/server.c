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
#define HEADERLEN 11

Message* list_reply(int);
Message* get_reply(bool);
Message* put_reply();
void *connectionFunction(void *);
void sendListReply(int);
char* listDirectory();
void messageAction(Message*, int);
void sendGetReply(Message*, int);

Message* list_reply(int payloadLength){
    Message *listReplyMessage = (Message *) malloc(sizeof(Message));
    strcpy(listReplyMessage->protocol, "myftp");
    listReplyMessage->type = 0xA2;
    listReplyMessage->length = HEADERLEN + payloadLength;
	//listReplyMessage->length = htonl(listReplyMessage->length);
    return listReplyMessage;
}

Message* get_reply(bool existance){
    Message *getReplyMessage = (Message *) malloc(sizeof(Message));
    if(existance)
        getReplyMessage->type = 0xB2;
    else
        getReplyMessage->type = 0xB3;
    strcpy(getReplyMessage->protocol, "myftp");
    getReplyMessage->length = HEADERLEN;
    return getReplyMessage;
}

Message* put_reply(){
    Message *putReplyMessage = (Message *) malloc(sizeof(Message));
    strcpy(putReplyMessage->protocol, "myftp");
    putReplyMessage->type = 0xC2;
    putReplyMessage->length = HEADERLEN;
    return putReplyMessage;
}



/*
 * Worker thread performing receiving and outputing messages
 */



void sendGetReply(Message* receivedFromClient, int clientSocket){
	int filenameLength = receivedFromClient->length - 11;
	char filename[filenameLength];
	int receivedLength = recv(clientSocket, filename, filenameLength, 0);
	if(receivedLength < 0)
		printf("Connection Error: %s (Errno:%d)\n", strerror(errno), errno);
	else
		filename[receivedLength] = '\0';
	// printf("File Name: %s\n", filename);

	// File Path Parsing
	char filePath[receivedLength + 2 + 4];
	// Only for Linux/Unix Environment
	strcpy(filePath, "data/");
	strcat(filePath, filename);
	printf("File Path: %s\n", filePath);
	//

	FILE *fp = fopen(filePath, "rb");
	if (ENOENT == errno){
		Message *getErrorReplyMessage = get_reply(false);
		sendMessage(getErrorReplyMessage, clientSocket);
		return;
	}
	fseek(fp, 0, SEEK_END); 	// seek to end of file
	int fileSize = ftell(fp); 	// get current file pointer
	fseek(fp, 0, SEEK_SET); 	// seek back to beginning of file

	Message *getReplyMessage = get_reply(true);
	sendMessage(getReplyMessage, clientSocket);

	Message *fileDataMessage = file_data(fileSize);
	sendMessage(fileDataMessage, clientSocket);

	int wordSize = 1024;
	while(!feof(fp)){
		char words[wordSize];
		size_t bytes_read = fread(words, sizeof(char), wordSize - 1, fp);
		Message *fileDataMessage = file_data(bytes_read * sizeof(char));
		sendMessage(fileDataMessage, clientSocket);
		send(clientSocket, words, bytes_read * sizeof(char), 0);
	}
	printf("File sending is finished\n");
	fclose(fp);
}

void *connectionFunction(void *sd){
	char buffer[100];
	int length;
	int client_sd = *((int *) sd);
	// printf("RECEIVING SOMETHING\n");
	Message* receivedMessage;
	// Client is now connected to server
	// recv(client_sd, (void *) receivedMessage, sizeof(receivedMessage), 0);
	receivedMessage = receiveMessage(client_sd);
	printf("Received from: %s 0x%02X %d\n", receivedMessage->protocol, receivedMessage->type, receivedMessage->length);
	messageAction(receivedMessage, client_sd);
	close(client_sd);
	pthread_exit(NULL);
}

void messageAction(Message* recv_msg, int client_sd){
	int header_len = (int)sizeof(recv_msg->protocol) + (int)sizeof(recv_msg->type) + (int)sizeof(recv_msg->length);
	if(strcmp(recv_msg->protocol, "myftp") != 0){
		char *returnMessage = "protocol message not myftp. Error from messageAction";
		printf("%s\n", returnMessage);
		send(client_sd, returnMessage, strlen(returnMessage), 0);
		printf("send done\n");
	}else{
		if((int)recv_msg->type == (int)0xA1 && recv_msg->length == 11){
			printf("Get LIST_REQUEST\n");
			
			
			DIR *dir;
			struct dirent *ptr_dir;
			dir = opendir("data/");
			int fileListSize = 1;
			while((ptr_dir = readdir(dir))!=NULL){
				fileListSize += strlen(ptr_dir->d_name);
				fileListSize += 1;
			}
			closedir(dir);
			char *fileList = (char *)malloc(fileListSize);
			dir = opendir("data/");
			int fileListStrPtr = 0;
			while((ptr_dir = readdir(dir))!=NULL){
				memcpy(fileList + fileListStrPtr, ptr_dir->d_name, strlen(ptr_dir->d_name));
				fileListStrPtr += strlen(ptr_dir->d_name);
				fileList[fileListStrPtr] = '\n';
				fileListStrPtr ++;
			}
			fileList[fileListSize - 1] = '\0';		
			
			Message *listReplyMessage = list_reply(strlen(fileList));
			sendMessage(listReplyMessage,client_sd);

			int length = send(client_sd, fileList, htonl(strlen(fileList)), 0);
			free(fileList);
			if(length < 0){
				printf("Connection Error: %s (Errno:%d)\n", strerror(errno), errno);
			}
			
			
			printf("LIST_REPLY DONE\n");
		}
		// GET_REQUEST
		else if((int)recv_msg->type == (int)0xB1){
			printf("Get GET_REQUEST\n");
			sendGetReply(recv_msg, client_sd);
			printf("GET_REPLY DONE\n");
		}
		// PUT_REQUEST
		else if((int)recv_msg->type == (int)0xC1){
			printf("Get PUT_REQUEST\n");
			//uploadFileFromClient(recv_msg, client_sd);
			printf("PUT_REPLY DONE\n");
		}else{
			printf("Type recv error. Line 96\n");
		}
	}
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
	long val = 1;
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(long)) == -1){
		perror("setsockopt");
		exit(1);
	}
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
