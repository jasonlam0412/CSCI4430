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