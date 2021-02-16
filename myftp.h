#ifndef _MYFTP_H_
#define _MYFTP_H_

typedef struct message_s {
    unsigned char protocol[6];
    unsigned char type;
    unsigned int length;
} __attribute__ ((packed)) Message;

ftpMessage* list_request();

ftpMessage* list_reply(int);

ftpMessage* get_request(int);

ftpMessage* get_reply(bool);

ftpMessage* put_request(int);

ftpMessage* put_reply();

ftpMessage* file_data(int);

void sendOutFTPMessage(ftpMessage *, int);

ftpMessage* receiveFTPMessage(int);