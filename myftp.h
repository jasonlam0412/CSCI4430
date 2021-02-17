#ifndef _MYFTP_H_
#define _MYFTP_H_

typedef struct message_s {
    unsigned char protocol[6];
    unsigned char type;
    unsigned int length;
} __attribute__ ((packed)) Message;

Message* list_request();

Message* list_reply(int);

Message* get_request(int);

Message* get_reply(bool);

Message* put_request(int);

Message* put_reply();

Message* file_data(int);

void sendOutFTPMessage(Message *, int);

Message* receiveFTPMessage(int);