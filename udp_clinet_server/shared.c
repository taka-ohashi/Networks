#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "shared.h"
#include "gethostbyname6.h"
#include "server.h" 

/* * * * * * full send and receive * * * * */
void fullsend(int socketNum, char * packet, int len){
	if (send(socketNum, packet, len, 0) < 0){
		printf("sending packet error");
		exit(1);
	}
}

void fullrecv(int socketNum, char * packet, int * packetLen) {
	int recvLen = 0;
	uint16_t pLen = 0;
	recvLen = recv(socketNum, packet, PDULENSIZE, 0);
	if (recvLen < 0) {
		printf("recv() error: receiving packet error");
		exit(1);
	}
	if (recvLen == 0) {
		*packetLen = pLen;
		return;
	}
	memcpy(&pLen, packet, PDULENSIZE);
	pLen = ntohs(pLen);
	int yetToGet = pLen - recvLen;
	while (yetToGet > 0) {
		if ((recvLen = recv(socketNum, packet + (pLen - yetToGet), yetToGet, 0)) < 0) {
			printf("recv() error: receiving packet error");
			exit(1);
		}
		yetToGet -= recvLen;
	}
	*packetLen = pLen;
}



