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

void sendListReply(int clientSocket){
	char *listContent = listDirectory();

	// First send out list_reply structure to notify client
	ftpMessage *listReplyMessage = list_reply(strlen(listContent));
	// send(clientSocket, (void *) listReplyMessage, sizeof(*listReplyMessage), 0);
	sendOutFTPMessage(listReplyMessage,clientSocket);

	int length = send(clientSocket, listContent, strlen(listContent), 0);
	free(listContent);
	if(length < 0){
		printf("Connection Error: %s (Errno:%d)\n", strerror(errno), errno);
	}
}

char* listDirectory(){
	struct dirent *directoryEntry;
	char *fileList = (char *) malloc(sizeof(char));
	int stringTotalLength = 0;
	strcpy(fileList, "");
	DIR* dir = opendir("data");
	if (ENOENT == errno){
		mkdir("data", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		dir = opendir("data");
	}
	while((directoryEntry = readdir(dir)) != NULL){
		if(strcmp(directoryEntry->d_name, ".") == 0 || strcmp(directoryEntry->d_name, "..") == 0)
			continue;

		char *writeIn = (char *) malloc(sizeof(char *) * (strlen(directoryEntry->d_name) + 1 + sizeof('\n')));
		strcpy(writeIn, directoryEntry->d_name);
		strcat(writeIn,"\n");
		stringTotalLength += (strlen(writeIn));
		// printf(" listDirectory() Function: %s",writeIn);
		char *tempString = (char *) realloc(fileList, sizeof(char *) * (stringTotalLength));
		fileList = tempString;
		strcat(fileList, writeIn);
	}
	closedir(dir);
	return fileList;
}


void sendGetReply(ftpMessage* receivedFromClient, int clientSocket){
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
		ftpMessage *getErrorReplyMessage = get_reply(false);
		sendOutFTPMessage(getErrorReplyMessage, clientSocket);
		return;
	}
	fseek(fp, 0, SEEK_END); 	// seek to end of file
	int fileSize = ftell(fp); 	// get current file pointer
	fseek(fp, 0, SEEK_SET); 	// seek back to beginning of file

	ftpMessage *getReplyMessage = get_reply(true);
	sendOutFTPMessage(getReplyMessage, clientSocket);

	ftpMessage *fileDataMessage = file_data(fileSize);
	sendOutFTPMessage(fileDataMessage, clientSocket);

	int wordSize = 1024;
	while(!feof(fp)){
		char words[wordSize];
		size_t bytes_read = fread(words, sizeof(char), wordSize - 1, fp);
		ftpMessage *fileDataMessage = file_data(bytes_read * sizeof(char));
		sendOutFTPMessage(fileDataMessage, clientSocket);
		send(clientSocket, words, bytes_read * sizeof(char), 0);
	}
	printf("File sending is finished\n");
	fclose(fp);
}

void uploadFileFromClient(ftpMessage* receivedFromClient, int clientSocket){
	ftpMessage *putReplyMessage = put_reply();
	sendOutFTPMessage(putReplyMessage, clientSocket);
	char filePath[receivedFromClient->length - 11];
	int length = recv(clientSocket, filePath, receivedFromClient->length - 11, 0);
	filePath[length] = '\0';
	char targetFile[100] = {'\0'};
	printf("File Path: %s\n", filePath);

	strcpy(targetFile, "data/");
	strcat(targetFile, filePath);
	printf("File Path: %s\n", targetFile);
	FILE *fp = fopen(targetFile, "w+b");

	// Receive file data with size
	// ftpMessage fileData;
	// recv(clientSocket, (void *) &fileData, sizeof(fileData), 0);
	ftpMessage* fileData = receiveFTPMessage(clientSocket);


	if(!(strcmp(fileData->protocol, "myftp") == 0 && fileData->type == (int)0xFF)){
		printf("File data structure error!\n");
		exit(0);
	}
	int fileDataLength = fileData->length - 11;
	printf("Waiting file... : %d\n", fileDataLength);

	// wordSize depends on connection, it can be changed
	int wordSize = 1024;
	while(fileDataLength > 0){
		char words[wordSize];

		ftpMessage* fileSizeData = receiveFTPMessage(clientSocket);
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

void messageAction(ftpMessage* receivedFromClient, int clientSocket){
	int messageLength = (int)sizeof(receivedFromClient->protocol) + (int)sizeof(receivedFromClient->type) + (int)sizeof(receivedFromClient->length);
	printf("%i\n", messageLength);
	// check protocol error
	if(strcmp(receivedFromClient->protocol, "fubar") != 0){
		char *errorMessage = "Wrong protocol to handle";
		printf("%s\n", errorMessage);
		send(clientSocket, errorMessage, strlen(errorMessage), 0);
	}
	
	//???? 
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