# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <pthread.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#include "myftp.h"
#define HEADERLEN 11

Message* file_data(int fileSize){
    Message *fileDataMessage = (Message *) malloc(sizeof(Message));
    strcpy(fileDataMessage->protocol, "myftp");
    fileDataMessage->type = 0xFF;
    fileDataMessage->length = HEADERLEN + fileSize;
    return fileDataMessage;
}

int sendn(int sd, void *buf, int buf_len){
	int n_left = buf_len;
	int n;
	while (n_left > 0){
		if ((n= send(sd, buf + (buf_len - n_left), n_left, 0)) < 0){
			if(errno == EINTR){
				n = 0;
			}else{
				return -1;
			}
			
		}else if( n == 0){
			return 0;
		}
		n_left -= n;
		
	}
	return buf_len;
}

int recvn(int sd, void *buf, int buf_len){
	int n_left = buf_len;
	int n;
	while (n_left > 0){
		if ((n= recv(sd, buf + (buf_len - n_left), n_left, 0)) < 0){
			if(errno == EINTR){
				n = 0;
			}else{
				return -1;
			}
			
		}else if( n == 0){
			return 0;
		}
		n_left -= n;
	}
	return buf_len;
}

void sendMessage(Message *data, int sd){
    // printf("BEFORE SEND: %d\n", data->length);
    data->length = htonl(data->length);
	int length = send(sd, (void *) data, sizeof(*data), 0);
    // printf("AFTER SEND: %d\n", data->length);
	free(data);
	if(length < 0){
		printf("Connection Error: %s (Errno:%d)\n", strerror(errno), errno);
	}
}

Message* receiveMessage(int sd){
    Message *receiveMessage = (Message *) malloc(sizeof(Message));
	int length = recv(sd, (void *) receiveMessage, sizeof(*receiveMessage), 0);
    receiveMessage->length = ntohl(receiveMessage->length);
	if(length < 0){
		printf("Connection Error: %s (Errno:%d)\n", strerror(errno), errno);
	}
    return receiveMessage;
}