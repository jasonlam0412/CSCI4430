typedef struct message_s {
    unsigned char protocol[6];
    unsigned char type;
    unsigned int length;
	unsigned char payload[500];
	
} __attribute__ ((packed)) Message;

Message* file_data(int);
int sendn(int, void *, int);
int recvn(int, void *, int);
void sendMessage(Message *, int);
Message* receiveMessage(int);
