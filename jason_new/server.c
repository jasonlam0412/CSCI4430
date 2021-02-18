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
    strcpy(listReplyMessage->protocol, "fubar");
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
    strcpy(getReplyMessage->protocol, "fubar");
    getReplyMessage->length = 11;
    return getReplyMessage;
}

Message* put_reply(){
    Message *putReplyMessage = (Message *) malloc(sizeof(Message));
    strcpy(putReplyMessage->protocol, "fubar");
    putReplyMessage->type = 0xC2;
    putReplyMessage->length = 11;
    return putReplyMessage;
}


void sendGetReply(Message* receivedFromClient, int clientSocket){
	int fileNameSize = receivedFromClient->length - 11;
	char filename[fileNameSize];
	int receivedLength = recv(clientSocket, filename, fileNameSize, 0);
	if(receivedLength < 0)
		printf("Connection Error: %s (Errno:%d)\n", strerror(errno), errno);
	else
		filename[receivedLength] = '\0';
	//testing
	printf("File Name: %s\n", filename);

	// File Path Parsing
	char filePath[receivedLength + 6];
	// Only for Linux/Unix Environment
	strcpy(filePath, "data/");
	strcat(filePath, filename);
	
	//testing
	printf("File Path: %s\n", filePath);
	//file exist
	FILE *fp = fopen(filePath, "rb");	//read binary file
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




void messageAction(Message* receivedFromClient, int clientSocket){
	int messageLength = (int)sizeof(receivedFromClient->protocol) + (int)sizeof(receivedFromClient->type) + (int)sizeof(receivedFromClient->length);
	printf("%i\n", messageLength);
	// check protocol error
	if(strcmp(receivedFromClient->protocol, "fubar") != 0){
		char *errorMessage = "Wrong protocol to handle";
		printf("%s\n", errorMessage);
		send(clientSocket, errorMessage, strlen(errorMessage), 0);
	}
	
	// check messageLength error
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
			sendMessage(listReplyMessage,clientSocket);

			int length = send(clientSocket, fileList, strlen(fileList), 0);
			free(fileList);
			if(length < 0){
				printf("Connection Error: %s (Errno:%d)\n", strerror(errno), errno);
			}
			
		}
		// GET_REQUEST
		else if((int)receivedFromClient->type == (int)0xB1){
			int filenameLength = receivedFromClient->length - 11;
			char filename[filenameLength];
			int receivedLength = recv(clientSocket, filename, filenameLength, 0);
			if(receivedLength < 0)
				printf("Connection Error: %s (Errno:%d)\n", strerror(errno), errno);
			else
				filename[receivedLength] = '\0';
			char filePath[receivedLength + 2 + 4];
			strcpy(filePath, "data/");
			strcat(filePath, filename);
			printf("File Path: %s\n", filePath);
			FILE *fp = fopen(filePath, "rb");
			if (fp == 0){
				Message *getErrorReplyMessage = get_reply(false);
				sendMessage(getErrorReplyMessage, clientSocket);
				return;
			}
			fseek(fp, 0, SEEK_END); 	// seek to end of file
			int fileSize = ftell(fp); 	// get current file pointer
			fseek(fp, 0, SEEK_SET); 	

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
		// PUT_REQUEST
		else if((int)receivedFromClient->type == (int)0xC1){
	
			//send message for connection put ensure
			Message *putReplyMessage = put_reply();
			sendMessage(putReplyMessage, clientSocket);
			char filePath[receivedFromClient->length - 11];
			int length = recv(clientSocket, filePath, receivedFromClient->length - 11, 0);
			filePath[length] = '\0';
			char targetFile[100] = {'\0'};
			printf("File Path: %s\n", filePath);

			strcpy(targetFile, "data/");
			strcat(targetFile, filePath);
			printf("File Path: %s\n", targetFile);
			FILE *fp = fopen(targetFile, "w+b");

			Message* fileData = receiveMessage(clientSocket);

			if(!(strcmp(fileData->protocol, "fubar") == 0 && fileData->type == (int)0xFF)){
				printf("File message error!\n");
				exit(0);
			}
			int fileDataLength = fileData->length - 11;
			printf("Waiting file... : %d\n", fileDataLength);

			// wordSize depends on connection, it can be changed
			int wordSize = 1024;
			while(fileDataLength > 0){
				char words[wordSize];
				Message* fileSizeData = receiveMessage(clientSocket);
				if(fileSizeData->length - 11 > wordSize)
					fileSizeData->length = ntohl(fileSizeData->length);
				printf("%d\n", fileSizeData->length - 11);
				int length = recv(clientSocket, words, fileSizeData->length - 11, 0);
				fileDataLength -= fileSizeData->length - 11;
				fwrite(words, sizeof(char), fileSizeData->length - 11, fp);
			}
			printf("File has been received and saved!!!\n");
			fclose(fp);
			
		}
	}
}

void *connectionFunction(void *client_sd){
	char buffer[100];
	int length;
	int clientSocket = *((int *) client_sd);
	Message* receivedMessage;
	receivedMessage = receiveMessage(clientSocket);
	printf("Received from sender: %s 0x%02X %d\n", receivedMessage->protocol, receivedMessage->type, receivedMessage->length);
	messageAction(receivedMessage, clientSocket);
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
