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

#include "cclient.h"
#include "shared.h"
#include "gethostbyname6.h"

/* * * * * * * * main * * * * * * * */
int main(int argc, char * argv[]) {
	checkArgs(argc, argv);
	int socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);
	send_flag1(argv[1], socketNum);
	recv_flag2_or_flag3(socketNum);
	send_and_receieve_packets(argv[1], socketNum);
	return 0;
}

/* * * * * * * * connection setup * * * * * * * */
int tcpClientSetup(char * serverName, char * port, int debugFlag){
	int socket_num;
	uint8_t * ipAddress = NULL;
	struct sockaddr_in6 server;      
	if ((socket_num = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
		perror("socket call");
		exit(-1);
	}
	if (debugFlag){
		printf("Connecting to server on port number %s\n", port);
	}
	server.sin6_family = AF_INET6;
	server.sin6_port = htons(atoi(port));
	if ((ipAddress = getIPAddress6(serverName, &server)) == NULL){
		exit(-1);
	}
	printf("server ip address: %s\n", getIPAddressString(ipAddress));
	if(connect(socket_num, (struct sockaddr*)&server, sizeof(server)) < 0){
		perror("connect call");
		exit(-1);
	}
	if (debugFlag){
		printf("Connected to %s IP: %s Port Number: %d\n", serverName, getIPAddressString(ipAddress), atoi(port));
	}
	return socket_num;
}

void send_flag1(char * handleName, int socketNum) {
	char packet[MAX_PACKET_SIZE];
	uint8_t flag1 = FLAG1;
	uint8_t handleLen = strlen(handleName);
	uint16_t pdu_len = htons(CHATHEADERSIZE + HANDLELENSIZE + handleLen);
	memcpy(packet, &pdu_len, PDULENSIZE);
	memcpy(packet + PDULENSIZE, &flag1, FLAGSIZE);
	memcpy(packet + PDULENSIZE + FLAGSIZE, &handleLen, HANDLELENSIZE);
	memcpy(packet + PDULENSIZE + FLAGSIZE + HANDLELENSIZE, handleName, handleLen);
	printf("\n%s\n", "sending flag1");
	fullsend(socketNum, packet, ntohs(pdu_len));
}


void recv_flag2_or_flag3(int socketNum) {
	char packet[MAX_PACKET_SIZE];
	int messageLen = 0;
	fullrecv(socketNum, packet, &messageLen);
	if (messageLen == 0){
		printf("Connection error: server exited (^C)");
	}
	run_recv_error_check(packet);
}

void run_recv_error_check(char * packet){ 
	uint8_t flag = 0;
	memcpy(&flag, packet + FLAGPOSITION, FLAGSIZE);
	printf("flag received: %i\n\n", flag);
	if (flag == FLAG3){
		printf("Error on initial packet: handle already exists");
		exit(1);
	}
	else if (flag != FLAG2){
		printf("Error on initial packet: incorrect packet type");
		exit(1);
	}
}

/* * * * * * * * error handling * * * * * * * */
void checkArgs(int argc, char *argv[]){
	if (argc != 4){
		printf("usage: %s handle host-name port-number\n", argv[0]);
		exit(1);
	}
	checkHandleName(argv);
}

void checkHandleName(char *argv[]){
	if (isdigit(argv[1][0])){
		printf("Invalid handle name: handle cannot start with a number\n");
		exit(1);
	}
	if (strlen(argv[1]) > 100) {
		printf("Invalid handle name: longer than 100\n");
		exit(1);
	}
}

/* * * * * * * * while(1) * * * * * * * */
void send_and_receieve_packets(char * clientHandle, int serverSocket) {
	fd_set fdvar;
	while(1) {
		int highestNumberFd = serverSocket;
		FD_ZERO(&fdvar);
		FD_SET(STDIN_FILENO, &fdvar);
		FD_SET(serverSocket, &fdvar);
		printf("$: ");
		fflush(stdout);
		if (select(highestNumberFd + 1, &fdvar, NULL, NULL, NULL) < 0){
			printf("select call error");
			exit(1);
		}
		if (FD_ISSET(STDIN_FILENO, &fdvar) != 0) {
			char * line = parseCommandLine(stdin);
			differentiateCommand(line, clientHandle, serverSocket);
			free(line);
		}
		if (FD_ISSET(serverSocket, &fdvar) != 0) {
			retreiveMessageFromServer(serverSocket);
		}
	}
}

void retreiveMessageFromServer(int serverSocket) {
	char packet[MAX_PACKET_SIZE];
	int packetLen = 0;
	uint8_t flag = 0;
	fullrecv(serverSocket, packet, &packetLen);
	if (packetLen == 0) {
		printf("Server terminated\n");
		exit(1);
	}
	memcpy(&flag, packet + FLAGPOSITION, FLAGSIZE);
	if (flag == FLAG4){recv_flag4(packet);}
	else if (flag == FLAG5){recv_flag5(packet);}
	else if (flag == FLAG7){recv_flag7(packet);}
	else if (flag == FLAG9){recv_flag9();}
	else if (flag == FLAG11){recv_flag11_12_13(serverSocket, packet);}
}


void recv_flag7(char * buf) {
	uint8_t handleLen = 0;
	char handle[MAXHANDLE];
	memcpy(&handleLen, buf + HANDLEPOSITION, HANDLELENSIZE);
	memcpy(handle, buf + HANDLELABELPOSITION, (int) handleLen);
	handle[handleLen] = 0;
	printf("Client with handle %s does not exist\n", handle);
}

/* * * * * * * * parsing line * * * * * * * */
char *parseCommandLine(FILE *file) {
	int length_of_string = 10;
	int i = 0;
	char * temp1 = malloc(length_of_string * sizeof(char));
	int int_stdin;
	//int last_index;
	while ((int_stdin = fgetc(file)) != EOF && int_stdin != '\n') {
		temp1[i++] = int_stdin;
		//last_index = length_of_string - 1;
		if (i == (length_of_string - 1)) {
			length_of_string = length_of_string * 2;
			temp1 = realloc(temp1, length_of_string);
		}
	}
	temp1[i] = '\0';
	if (int_stdin == EOF) {
		free(temp1);
		temp1 = NULL;
	}
	return temp1;
}

void differentiateCommand(char * input, char * handle, int serverSocket) {
	char *token = NULL;
	int flag = 0;
	token = strtok(input, " ");
	if (token != NULL) {
		if (flag == 0) {
			if (token[0] != '%') {
				printf("%s\n", "invalid command error: %M, %B, %L");
				return;
			}
			flag++;
		}
		if ((token[1] == 'm') || (token[1] == 'M')){
			send_flag5(serverSocket, handle, strtok(NULL, ""));
		}
		else if ((token[1] == 'b') || (token[1] == 'B')){
			char *text = strtok(NULL, "");
			if (text == NULL) { 
				text = "\n";
			}
			send_flag4(serverSocket, handle, text);
		}
		else if ((token[1] == 'l') || (token[1] == 'L')){
			send_flag10(serverSocket);
		}
		else if ((token[1] == 'e') || (token[1] == 'E')){
			send_flag8(serverSocket);
		}
		else{
			printf("%s\n", "Invalid command error: %M, %B, %L");
		}
		token = strtok(NULL, " ");
	}
}

/* * * * * * * * %M * * * * * * * */
void send_flag5(int serverSocket, char * handle, char * inputBuf) {
	char *token = NULL;
	char *text = NULL;
	token = strtok(inputBuf, " ");
	uint8_t handleCount = atoi(token);
	if (handleCount > 9 || handleCount < 0) {
		printf("Destination handle number error: 0-9\n");
	}
	if (handleCount == 0) {
		char packet[MAX_PACKET_SIZE];
		char *target = token;
		uint8_t targetHandleLen = strlen(target);
		int packetSize = 0;
		int num = 0;
		int yetToGet;
		handleCount = 1;
		memcpy(packet, &handleCount, HANDLELENSIZE);
		memcpy(packet + HANDLELENSIZE, &targetHandleLen, HANDLELENSIZE);
		memcpy(packet + HANDLELENSIZE * 2, target, (int) targetHandleLen);
		text = strtok(NULL, "");
		if (text == NULL) {
			text = "\n";
		}
		packetSize = packetSize + HANDLELENSIZE + HANDLELENSIZE + targetHandleLen;
		yetToGet = strlen(text);
		while (yetToGet > 0) {
			char inputBuf[MAXTEXTSIZE + 1];
			int bytes_after_memcpy = 0;
			if (yetToGet >= MAXTEXTSIZE) {
				memcpy(inputBuf, text + num, MAXTEXTSIZE);
				num += MAXTEXTSIZE;
				yetToGet -= MAXTEXTSIZE;
				bytes_after_memcpy = MAXTEXTSIZE;
			} else {
				memcpy(inputBuf, text + num, yetToGet);
				num += yetToGet;
				bytes_after_memcpy = yetToGet;
				yetToGet = 0;
			}
			inputBuf[bytes_after_memcpy] = 0;
			fullsendmessage(serverSocket, handle, packet, packetSize, inputBuf, bytes_after_memcpy + 1);
		}
	} 
	else {
		int i = 0;
		int offset = 0;
		int num = 0;
		int yetToGet;
		char packet[MAX_PACKET_SIZE];
		memcpy(packet, &handleCount, HANDLELENSIZE);
		offset += HANDLELENSIZE;
		while(i < handleCount){
			char * dest = strtok(NULL, " ");
			uint8_t targetSize = strlen(dest);
			memcpy(packet + offset, &targetSize, HANDLELENSIZE);
			offset += HANDLELENSIZE;
			memcpy(packet + offset, dest, (int) targetSize);
			offset += targetSize;
			i++;
		}
		text = strtok(NULL, "");
		if (text == NULL) {
			text = "\n";
		}
		yetToGet = strlen(text);
		while (yetToGet > 0) {
			char inputBuf[MAXTEXTSIZE + 1];
			int bytes_after_memcpy = 0;
			if (yetToGet >= MAXTEXTSIZE) {
				memcpy(inputBuf, text + num, MAXTEXTSIZE);
				num += MAXTEXTSIZE;
				yetToGet -= MAXTEXTSIZE;
				bytes_after_memcpy = MAXTEXTSIZE;
			}
			else {
				memcpy(inputBuf, text + num, yetToGet);
				num += yetToGet; 
				bytes_after_memcpy = yetToGet;
				yetToGet = 0;
			}
			inputBuf[bytes_after_memcpy] = 0;
			bytes_after_memcpy++;
			fullsendmessage(serverSocket, handle, packet, offset, inputBuf, bytes_after_memcpy);
		}
	}
}

void fullsendmessage(int serverSocket, char * handle, char * packet, int packetSize, char * buf, int packetByteSize) {
	char sendBuf[MAX_PACKET_SIZE];
	uint8_t handleLen = strlen(handle);
	uint16_t packetLen = htons(CHATHEADERSIZE + HANDLELENSIZE + handleLen + packetSize + packetByteSize);
	uint8_t flag = FLAG5;
	memcpy(sendBuf, &packetLen, PDULENSIZE);
	memcpy(sendBuf + PDULENSIZE, &flag, FLAGSIZE);
	memcpy(sendBuf + PDULENSIZE + FLAGSIZE, &handleLen, HANDLELENSIZE);
	memcpy(sendBuf + PDULENSIZE + FLAGSIZE + HANDLELENSIZE, handle, handleLen);
	memcpy(sendBuf + PDULENSIZE + FLAGSIZE + HANDLELENSIZE + handleLen, packet, packetSize);
	memcpy(sendBuf + PDULENSIZE + FLAGSIZE + HANDLELENSIZE + handleLen + packetSize, buf, packetByteSize);
	fullsend(serverSocket, sendBuf, ntohs(packetLen));
}

void recv_flag5(char * temp) {
	uint8_t handleLen = 0;
	uint8_t numHandles = 0; 
	char handle[MAXHANDLE];
	int offsetSize = 0;
	int i = 0;
	int temp_int;
	memcpy(&handleLen, temp + HANDLEPOSITION, HANDLELENSIZE);
	offsetSize = offsetSize + HANDLELENSIZE + HANDLEPOSITION;
	temp_int = (int) handleLen;
	memcpy(handle, temp + HANDLELABELPOSITION, temp_int);
	handle[handleLen] = 0;
	offsetSize += handleLen;
	memcpy(&numHandles, temp + offsetSize, HANDLELENSIZE);
	offsetSize += HANDLELENSIZE;
	i = 0;
	while (i < numHandles){
		uint8_t targetSize = 0;
		memcpy(&targetSize, temp + offsetSize, HANDLELENSIZE);
		offsetSize += targetSize + HANDLELENSIZE;
		i++;
	}
	temp = temp + offsetSize;
	printf("%s: %s\n", handle, temp);
}

/* * * * * * * * %B * * * * * * * */
void send_flag4(int serverSocket, char * handle, char * text) {
	int counter = 0;
	int bytesLeft = strlen(text);
	char newPacket[MAX_PACKET_SIZE];
	uint8_t handleLen;
	uint16_t packetLen;
	uint8_t flag;
	while (bytesLeft > 0) {
		char tempBuf[MAXTEXTSIZE + 1];
		int bytesSize = 0;
		if (bytesLeft >= MAXTEXTSIZE) {
			memcpy(tempBuf, text + counter, MAXTEXTSIZE);
			counter += MAXTEXTSIZE;
			bytesLeft -= MAXTEXTSIZE;
			bytesSize = MAXTEXTSIZE;
		} else {
			memcpy(tempBuf, text + counter, bytesLeft);
			counter += bytesLeft;
			bytesSize = bytesLeft;
			bytesLeft = 0;
		}
		tempBuf[bytesSize] = 0;
		bytesSize++;
		handleLen = strlen(handle);
		packetLen = htons(CHATHEADERSIZE + HANDLELENSIZE + handleLen + bytesSize);
		flag = FLAG4;
		memcpy(newPacket, &packetLen, PDULENSIZE);
		memcpy(newPacket + PDULENSIZE, &flag, FLAGSIZE);
		memcpy(newPacket + PDULENSIZE + FLAGSIZE, &handleLen, HANDLELENSIZE);
		memcpy(newPacket + PDULENSIZE + FLAGSIZE + HANDLELENSIZE, handle, handleLen);
		memcpy(newPacket + PDULENSIZE + FLAGSIZE + HANDLELENSIZE + handleLen, tempBuf, bytesSize);
		fullsend(serverSocket, newPacket, ntohs(packetLen));
	}
}

void recv_flag4(char * buf) {
	char handleName[MAXHANDLE];
	uint8_t handleLen = 0;
	memcpy(&handleLen, buf + HANDLEPOSITION, HANDLELENSIZE);
	memcpy(&handleName, buf + HANDLEPOSITION + HANDLELENSIZE, handleLen);
	printf("%s: %s\n", handleName, buf + HANDLEPOSITION + HANDLELENSIZE + handleLen);
}

/* * * * * * * * %L * * * * * * * */
void send_flag10(int socketNum) {
	char newPacket[MAX_PACKET_SIZE];
	uint16_t packetLen = htons(CHATHEADERSIZE);
	uint8_t flag = FLAG10;
	memcpy(newPacket, &packetLen, PDULENSIZE);
	memcpy(newPacket + FLAGPOSITION, &flag, FLAGSIZE);
	fullsend(socketNum, newPacket, CHATHEADERSIZE);
}

void recv_flag11_12_13(int serverSocket, char * buf) {
	uint32_t numHandles = 0;
	char packet[MAX_PACKET_SIZE];
	int packetLen = 0;
	uint8_t flag = 0;
	int i = 0;
	memcpy(&numHandles, buf + HANDLEPOSITION, HANDLERESSIZE);
	printf("Number of clients: %u\n", ntohl(numHandles));
	while (i < ntohl(numHandles)){
		print_all_handles(serverSocket);
		i++;
	}
	fullrecv(serverSocket, packet, &packetLen);
	if (packetLen == 0) {
		printf("Server termination error (^C) \n");
		exit(1);
	}
	memcpy(&flag, packet + FLAGPOSITION, FLAGSIZE); 
	if (flag != FLAG13) {
		printf("Handle count request errpr\n");
	}
}

void print_all_handles(int serverSocket) {
	uint8_t handleLen = 0;
	int packetLen = 0;
	char packet[MAX_PACKET_SIZE];
	char handleName[MAXHANDLE];
	fullrecv(serverSocket, packet, &packetLen);
	if (packetLen == 0) { 
		printf("Server termination error\n");
		exit(1);
	}
	memcpy(&handleLen, packet + HANDLEPOSITION, HANDLELENSIZE);
	memcpy(handleName, packet + HANDLELABELPOSITION, handleLen);
	handleName[handleLen] = 0;
	printf("\t%s\n", handleName);
} 

/* * * * * * * * %E * * * * * * * */
void send_flag8(int socketNum) {
	char newPacket[MAX_PACKET_SIZE];
	uint16_t packetLen = htons(CHATHEADERSIZE);
	uint8_t flag = FLAG8;
	memcpy(newPacket, &packetLen, PDULENSIZE);
	memcpy(newPacket + FLAGPOSITION, &flag, FLAGSIZE);
	fullsend(socketNum, newPacket, CHATHEADERSIZE);
}

void recv_flag9(){
	printf("Will Exit\n");
	exit(1);
}






