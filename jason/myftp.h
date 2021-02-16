typedef struct message_s {
    unsigned char protocol[6];
    unsigned char type;
    unsigned int length;
	unsigned char payload[500];
	
} __attribute__ ((packed)) P_message;