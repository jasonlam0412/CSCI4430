#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <signal.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>

#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "myftp.h"
#define HEADERLEN 11


Message* list_request();
Message* get_request(int);
Message* put_request(int);


Message* list_request(){
    Message *listRequestMessage = (Message *) malloc(sizeof(Message));
    strcpy(listRequestMessage->protocol, "myftp");
    listRequestMessage->type = 0xA1;
    listRequestMessage->length = HEADERLEN;
    return listRequestMessage;
}

Message* get_request(int payloadLength){
    Message *getRequestMessage = (Message *) malloc(sizeof(Message));
    strcpy(getRequestMessage->protocol, "myftp");
    getRequestMessage->type = 0xB1;
    getRequestMessage->length = HEADERLEN + payloadLength;
    return getRequestMessage;
}

Message* put_request(int payloadLength){
    Message *putRequestMessage = (Message *) malloc(sizeof(Message));
    strcpy(putRequestMessage->protocol, "myftp");
    putRequestMessage->type = 0xC1;
    putRequestMessage->length = HEADERLEN + payloadLength;
    return putRequestMessage;
}





/*
 * Worker thread performing receiving and outputing messages
 */


int main(int argc, char **argv) {
  /*if(connect(sd,(struct sockaddr *)&server_addr,sizeof(server_addr))<0){
    printf("connection error: %s (Errno:%d)\n",strerror(errno),errno);
    exit(0);
    }*/
	char IPaddress[15];
	assert(argc >= 3 && argc <= 5);
    int portNumber = atoi(argv[2]);
    strcpy(IPaddress, argv[1]);
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1){
        printf("Socket descriptor creation error.\n");
        exit(0);
    }
	struct sockaddr_in server_addr;
	pthread_t worker;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(IPaddress);
	server_addr.sin_port = htons(portNumber);
	if (connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
	printf("connection error: %s (Errno:%d)\n", strerror(errno), errno);
	exit(0);
	}
	////
	char buff[100];
	if(strcmp(argv[3], "list") == 0){
		Message *listRequestMessage = list_request();
		sendMessage(listRequestMessage,sd);
		Message* reply = receiveMessage(sd);
		if(strcmp(reply->protocol, "myftp") == 0 && reply->type == 0xA2){
			char replyMessage[reply->length - 11];
			int length = recvn(sd, replyMessage, reply->length - 11);
			if(length < 0)
				printf("Connection Error: %s (Errno:%d)\n", strerror(errno), errno);
			else{
				replyMessage[length] = '\0';
				printf("%s", replyMessage);
			}
		} else {
			//error in protocol and type
			printf("Server reply error. Wrong packet received.");
		}
	}else if(strcmp(argv[3], "get") == 0){
		assert(argc == 5);
		//send request
		sendMessage(get_request(strlen(argv[4])),sd);
		//send file name
		sendn(sd, argv[4], strlen(argv[4]));
		Message *reply = receiveMessage(sd);
		if(strcmp(reply->protocol, "myftp") == 0 && reply->type == 0xB2){
			//save files
			Message *replyFile = receiveMessage(sd);
			if (strcmp(replyFile->protocol, "myftp") != 0 || replyFile->type != 0xFF){
				printf("Server reply error.");
				exit(0);
			}
			int replyFilesize = replyFile->length - 11;
			FILE *fp = fopen(argv[4], "w+b");
			while(replyFilesize > 0){
				Message* fileSizeData = receiveMessage(sd);
				char words[1024];
				if(fileSizeData->length - 11 > 1024)
					fileSizeData->length = ntohl(fileSizeData->length);
				printf("%d\n", fileSizeData->length - 11);
				int length = recv(socketDescriptor, words, fileSizeData->length - 11, 0);
				fileDataLength -= fileSizeData->length - 11;
				fwrite(words, sizeof(char), fileSizeData->length - 11, fp);
			}
			printf("File has been received and saved!!!\n");
			fclose(fp);


		} else if (strcmp(reply->protocol, "myftp") == 0 && reply->type == 0xB3){
			printf("File does not exist!\n");
			exit(0);
		}

	}else if(strcmp(argv[3], "put") == 0){
		assert(argc == 5);
		FILE *fp = fopen(argv[4], "rb");
		if (fp == 0){
			printf("File cannot open\n");
			return 0;
		}else{
			printf("File opened\n");
		}
		fseek(fp, 0, SEEK_END); 	
		int fileSize = ftell(fp); 	
		fseek(fp, 0, SEEK_SET); 	
		Message *putRequestMessage = put_request(strlen(argv[4]));
		sendMessage(putRequestMessage, sd);
	}
	
	close(sd);
	
	return 0;
}
