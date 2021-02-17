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

#define IPADDR "127.0.0.1"
#define PORT 12345

Message* list_request();
Message* get_request(int);
Message* put_request(int);


Message* list_request(){
    Message *list_request_msg = (Message *) malloc(sizeof(Message));
    strcpy(list_request_msg->protocol, "fubar");
    list_request_msg->type = 0xA1;
    list_request_msg->length = 11;
    return list_request_msg;
}

Message* get_request(int payloadLength){
    Message *get_request_msg = (Message *) malloc(sizeof(Message));
    strcpy(get_request_msg->protocol, "fubar");
    get_request_msg->type = 0xB1;
    get_request_msg->length = 11 + payloadLength;
    return get_request_msg;
}

Message* put_request(int payloadLength){
    Message *put_request_msg = (Message *) malloc(sizeof(Message));
    strcpy(put_request_msg->protocol, "fubar");
    put_request_msg->type = 0xC1;
    put_request_msg->length = 11 + payloadLength;
    return put_request_msg;
}





/*
 * Worker thread performing receiving and outputing messages
 */


int main(int argc, char **argv) {
  /*if(connect(sd,(struct sockaddr *)&server_addr,sizeof(server_addr))<0){
    printf("connection error: %s (Errno:%d)\n",strerror(errno),errno);
    exit(0);
    }*/
	char ip_addr[15];
	assert(argc >= 3 && argc <= 5);
    int port_num = atoi(argv[2]);
    strcpy(ip_addr, argv[1]);
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1){
        printf("Socket descriptor creation error.\n");
        exit(0);
    }
	struct sockaddr_in server_addr;
	pthread_t worker;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip_addr);
	server_addr.sin_port = htons(port_num);
	if (connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
	printf("connection error: %s (Errno:%d)\n", strerror(errno), errno);
	exit(0);
	}
	////
	
	if(strcmp(argv[3], "list") == 0)
	{
		Message *list_request_msg = list_request();
		sendMessage(list_request_msg,sd);

		Message* reply = receiveMessage(sd);
		if(strcmp(reply->protocol, "fubar") == 0 && reply->type == 162){
			char Received[reply->length - 11];
			int length = recv(sd, Received, reply->length - 11, 0);
			if(length < 0)
				printf("Connection Error: %s (Errno:%d)\n", strerror(errno), errno);
			else{
				Received[length] = '\0';
				printf("%s", Received);
			}
		}
	}
	else if(strcmp(argv[3], "get") == 0){
		
		assert(argc == 5);
		Message *get_request_msg = get_request(strlen(argv[4]));
		
		sendMessage(get_request_msg,sd);

		send(sd, argv[4], strlen(argv[4]), 0);
		Message* reply_msg = receiveMessage(sd);

		if(strcmp(reply_msg->protocol, "fubar") == 0 && reply_msg->type == (int)0xB3){
			printf("File does not exist!\n");
			exit(0);
		}

		else if(strcmp(reply_msg->protocol, "fubar") == 0 && reply_msg->type == (int)0xB2){
			
			Message* fileData = receiveMessage(sd);


			if(!(strcmp(fileData->protocol, "fubar") == 0 && fileData->type == (int)0xFF)){
				printf("File data structure error!\n");
				exit(0);
			}
			int fileDataLength = fileData->length - 11;
			printf("Waiting file... : %d\n", fileDataLength);

			int MaxSize = 1024;
			FILE *fp = fopen(argv[4], "w+b");
			while(fileDataLength > 0){
				char words[MaxSize];
				Message* fileSizeData = receiveMessage(sd);
				
				if(fileSizeData->length - 11 > MaxSize)
					fileSizeData->length = ntohl(fileSizeData->length);
				printf("%d\n", fileSizeData->length - 11);
				int length = recv(sd, words, fileSizeData->length - 11, 0);
				fileDataLength -= fileSizeData->length - 11;
				fwrite(words, sizeof(char), fileSizeData->length - 11, fp);
			}
			printf("File has been received and saved!!!\n");
			fclose(fp);
		}
		

		// Send get request
		sendMessage(get_request_msg,sd);
	}
	else if(strcmp(argv[3], "put") == 0)
	{
		if(access(argv[4], F_OK) != 0){
            printf("file doesn't exit\n");
            exit(0);
        }
		FILE *fp = fopen(argv[4], "rb");
		if (fp == 0){
			printf("File cannot open\n");
			return 0;
		}
		fseek(fp, 0, SEEK_END); 	// seek to end of file
		int fileSize = ftell(fp); 	// get current file pointer
		fseek(fp, 0, SEEK_SET); 	// seek back to beginning of file
		
		Message* put_request_msg = put_request(strlen(argv[4]));
		sendMessage(put_request_msg, sd);
		send(sd, argv[4], strlen(argv[4]), 0);
		
		Message reply;
		recv(sd, (void *) &reply, sizeof(reply), 0);
		if(!(strcmp(reply.protocol, "fubar") == 0 && reply.type == (int)0xC2)){
			printf("Connection error!\n");
			exit(0);
		}
		
		Message *fileDataMessage = file_data(fileSize);
		sendMessage(fileDataMessage, sd);
		int wordSize = 1024;
		while(!feof(fp)){
			char words[wordSize];
			size_t bytes_read = fread(words, sizeof(char), wordSize - 1, fp);
			Message *fileDataMessage = file_data(bytes_read * sizeof(char));
			sendMessage(fileDataMessage,sd);
			send(sd, words, bytes_read * sizeof(char), 0);
		}
		printf("File sending is finished\n");
		fclose(fp);
		
		
	}
	
	close(sd);
	
	return 0;
}
