#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define header_size = 4+6+1

Message* list_request(){
    Message *listRequestMessage = (Message *) malloc(sizeof(Message));
    strcpy(listRequestMessage->protocol, "fubar");
    listRequestMessage->type = 0xA1;
    listRequestMessage->length = header_size;
    return listRequestMessage;
}

Message* list_reply(int payloadLength){
    Message *listReplyMessage = (Message *) malloc(sizeof(Message));
    strcpy(listReplyMessage->protocol, "fubar");
    listReplyMessage->type = 0xA2;
    listReplyMessage->length = header_size + payloadLength;
    return listReplyMessage;
}

Message* get_request(int payloadLength){
    Message *getRequestMessage = (Message *) malloc(sizeof(Message));
    strcpy(getRequestMessage->protocol, "fubar");
    getRequestMessage->type = 0xB1;
    getRequestMessage->length = header_size + payloadLength;
    return getRequestMessage;
}

Message* get_reply(bool existance){
    Message *getReplyMessage = (Message *) malloc(sizeof(Message));
    if(existance)
        getReplyMessage->type = 0xB2;
    else
        getReplyMessage->type = 0xB3;
    strcpy(getReplyMessage->protocol, "fubar");
    getReplyMessage->length = header_size;
    return getReplyMessage;
}

Message* put_request(int payloadLength){
    Message *putRequestMessage = (Message *) malloc(sizeof(Message));
    strcpy(putRequestMessage->protocol, "fubar");
    putRequestMessage->type = 0xC1;
    putRequestMessage->length = header_size + payloadLength;
    return putRequestMessage;
}

Message* put_reply(){
    Message *putReplyMessage = (Message *) malloc(sizeof(Message));
    strcpy(putReplyMessage->protocol, "fubar");
    putReplyMessage->type = 0xC2;
    putReplyMessage->length = header_size;
    return putReplyMessage;
}

Message* file_data(int fileSize){
    Message *fileDataMessage = (Message *) malloc(sizeof(Message));
    strcpy(fileDataMessage->protocol, "fubar");
    fileDataMessage->type = 0xFF;
    fileDataMessage->length = header_size + fileSize;
    return fileDataMessage;
}

void sendOutFTPMessage(Message *dataToSendOut, int socket){
    // printf("BEFORE SEND: %d\n", dataToSendOut->length);
    dataToSendOut->length = htonl(dataToSendOut->length);
	int length = send(socket, (void *) dataToSendOut, sizeof(*dataToSendOut), 0);
    // printf("AFTER SEND: %d\n", dataToSendOut->length);
	free(dataToSendOut);
	if(length < 0){
		printf("Connection Error: %s (Errno:%d)\n", strerror(errno), errno);
	}
}

Message* receiveFTPMessage(int socket){
    Message *receiveMessage = (Message *) malloc(sizeof(Message));
	int length = recv(socket, (void *) receiveMessage, sizeof(*receiveMessage), 0);
    receiveMessage->length = ntohl(receiveMessage->length);
	if(length < 0){
		printf("Connection Error: %s (Errno:%d)\n", strerror(errno), errno);
	}
    return receiveMessage;
}