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
#include "server.h"
#include "gethostbyname6.h"

/* * * * * * * * main * * * * * * * */
int main(int argc, char * argv[]) {
	int serverSocket = 0; //socket descriptor for the server socket
	int portNumber = 0;
	portNumber = checkArgs(argc, argv);
	serverSocket = tcpServerSetup(portNumber);
	processPackets(serverSocket);
	return 0;
}

/* * * * * * * * connection setup * * * * * * * */
int tcpServerSetup(int portNumber){
	int server_socket= 0;
	struct sockaddr_in6 server;
	socklen_t len= sizeof(server);
	server_socket= socket(AF_INET6, SOCK_STREAM, 0);
	if(server_socket < 0){
		perror("socket call");
		exit(1);
	}
	server.sin6_family= AF_INET6;         		
	server.sin6_addr = in6addr_any;
	server.sin6_port= htons(portNumber);         
	if (bind(server_socket, (struct sockaddr *) &server, sizeof(server)) < 0){
		perror("bind call");
		exit(-1);
	}
	if (getsockname(server_socket, (struct sockaddr*) &server, &len) < 0){
		perror("getsockname call");
		exit(-1);
	}
	if (listen(server_socket, BACKLOG) < 0){ 
		perror("listen call");
		exit(-1);
	}
	printf("Server Port Number %d \n", ntohs(server.sin6_port));
	return server_socket;
}

/* * * * * * * * error handling * * * * * * * */
int checkArgs(int argc, char *argv[])
{
	int portNumber = 0;
	if (argc > 2){
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	if (argc == 2){portNumber = atoi(argv[1]);}
	return portNumber;
}

/* * * * * * * * process packets * * * * * * * */
void processPackets(int serverSocket) {
	linkedList *list = initializelinkedList();
	fd_set fdvar;
	while(1) {
		linkedNode *current = list->head;
		int highestNumberFd = serverSocket;
		struct sockaddr_in6 clientInfo;
		int clientInfoSize;
		int clientSocket;
		FD_ZERO(&fdvar);
		FD_SET(serverSocket, &fdvar);
		setAll_and_findHighestNumberFd(current, highestNumberFd, serverSocket, fdvar);
		while (current != NULL) {
			FD_SET(current->socketNum, &fdvar);
			if (current->socketNum > highestNumberFd) {
				highestNumberFd = current->socketNum;
			}
			current = current->next;
		}
		if (select(highestNumberFd + 1, &fdvar, NULL, NULL, NULL) < 0){
			printf("select call error");
			exit(1);
		}
		if (FD_ISSET(serverSocket, &fdvar) != 0) {
			clientInfoSize = sizeof(clientInfo);
			//int clientSocket = acceptClient(serverSocket);
			if ((clientSocket = accept(serverSocket, (struct sockaddr *) &clientInfo, (socklen_t *) &clientInfoSize)) < 0) {
				printf("accept call error");
				exit(1);
			}
			linkedNode *newClient = initializelinkedNode(clientSocket);
			addTolinkedList(list, newClient);
		}
		current = list->head;
		while (current != NULL) {
			//differentiatepackets(current->socketNum, list); 
			if (FD_ISSET(current->socketNum, &fdvar) != 0) { //for multiple client set up
				differentiatepackets(current->socketNum, list); 
			}
			current = current->next; 
		}
	}
}

void setAll_and_findHighestNumberFd(linkedNode * head, int highestNumberFd, int serverSocket, fd_set fdvar){
	linkedNode * current = head;
	while (current != NULL){
		FD_SET(current -> socketNum, &fdvar);
		int sn = current -> socketNum;
		if (sn > highestNumberFd){
			highestNumberFd = sn;
		}
		current = current -> next;
	}
}

void differentiatepackets(int clientSocket, linkedList *list) {
	char packet[MAX_PACKET_SIZE];
	int messageLen = 0;
	uint8_t flag = 0;
	fullrecv(clientSocket, packet, &messageLen);
	if (messageLen == 0) {
		removeHandleFromList(list, clientSocket);
		close(clientSocket);
		return;
	}
	memcpy(&flag, packet + FLAGPOSITION, FLAGSIZE);
	if (flag == FLAG1){flag1Case(clientSocket, packet, list);}
	else if (flag == FLAG4){flag4Case(clientSocket, packet, list);}
	else if (flag == FLAG5){flag5Case(clientSocket, packet, list);}
	else if (flag == FLAG8){
		send_flag9(clientSocket);
		removeHandleFromList(list, clientSocket);
		close(clientSocket);
	}
	else if (flag == FLAG10){flag10Case(clientSocket, list);}
}



/* * * * * * * * FLAG Cases * * * * * * * */
void flag1Case(int clientSocket, char * packet, linkedList *list) {
	uint8_t handleLen = 0;
	char handleName[MAXHANDLE];
	uint16_t packetSize = htons(CHATHEADERSIZE);
	uint8_t flag;
	char newPacket[MAX_PACKET_SIZE];
	memcpy(&handleLen, packet + HANDLELENPOSITION, HANDLELENSIZE);
	memcpy(handleName, packet + HANDLEPOSITION, handleLen);
	handleName[handleLen] = 0;
	int output = checkHandleNameExistance(handleName, list);
	if (output == 0) {
		flag = FLAG2;
		linkedNode *node = findNode(list, clientSocket);
		node -> handleName = strdup(handleName);
		memcpy(newPacket, &packetSize, PDULENSIZE);
		memcpy(newPacket + FLAGPOSITION, &flag, FLAGSIZE);
		packetSize = ntohs(packetSize);
		fullsend(clientSocket, newPacket, packetSize);
	}
	else {
		flag = FLAG3;
		memcpy(newPacket, &packetSize, PDULENSIZE);
		memcpy(newPacket + FLAGPOSITION, &flag, FLAGSIZE);
		packetSize = ntohs(packetSize);
		fullsend(clientSocket, newPacket, packetSize);
		removeHandleFromList(list, clientSocket);
		close(clientSocket);
	}
}

/* * * * * * * * %B * * * * * * * */
void flag4Case(int clientSocket, char * message, linkedList *list) {
	char handle[MAXHANDLE];
	uint16_t packetLen = 0;
	uint8_t handleLen = 0;
	linkedNode *current = list->head;
	memcpy(&packetLen, message, PDULENSIZE);
	packetLen = ntohs(packetLen);
	memcpy(&handleLen, message + CHATHEADERSIZE, HANDLELENSIZE);
	memcpy(handle, message + CHATHEADERSIZE + HANDLELENSIZE, handleLen);
	handle[handleLen] = 0;
	while (current != NULL) {
		if (strcmp(current->handleName, handle) != 0) {
			fullsend(current->socketNum, message, (int) packetLen);
		}
		current = current->next;
	}	
}

/* * * * * * * * %M * * * * * * * */
void flag5Case(int clientSocket, char * packet, linkedList *list) {
	char handle[MAXHANDLE];
	uint16_t packetLen = 0;
	uint8_t handleLen = 0;
	uint8_t handleCount = 0;
	int offsetByte = 0;
	memcpy(&packetLen, packet, PDULENSIZE);
	packetLen = ntohs(packetLen);
	offsetByte += CHATHEADERSIZE;
	memcpy(&handleLen, packet + offsetByte, HANDLELENSIZE);
	offsetByte += HANDLELENSIZE;
	memcpy(handle, packet + offsetByte, handleLen);
	handle[handleLen] = 0;
	offsetByte += handleLen;
	memcpy(&handleCount, packet + offsetByte, HANDLELENSIZE);
	offsetByte += HANDLELENSIZE;
	int i = 0;
	while(i < handleCount){
		char target[MAXHANDLE];
		uint8_t currentHandleLen = 0;
		linkedNode *current = list->head;
		int invalidHandleFlag = 1;
		memcpy(&currentHandleLen, packet + offsetByte, HANDLELENSIZE);
		offsetByte += HANDLELENSIZE;
		memcpy(target, packet + offsetByte, currentHandleLen);
		target[currentHandleLen] = 0;
		offsetByte += currentHandleLen;
		while (current != NULL) {
			if (strcmp(current->handleName, target) == 0) {
				fullsend(current->socketNum, packet, (int) packetLen);
				invalidHandleFlag = 0;
			}
			current = current->next;
		}
		if (invalidHandleFlag) {
			send_flag7(clientSocket, target);
		}
		i++;
	}
}

/* * * * * * * * %E * * * * * * * */
void send_flag7(int clientSocket, char * target) {
	char newPacket[MAX_PACKET_SIZE];
	uint8_t flag = FLAG7;
	uint8_t handleLen = strlen(target);
	uint16_t packetLen = htons(CHATHEADERSIZE + HANDLELENSIZE + handleLen);
	memcpy(newPacket, &packetLen, PDULENSIZE);
	memcpy(newPacket + FLAGPOSITION, &flag, FLAGSIZE);
	memcpy(newPacket + HANDLE_POS, &handleLen, HANDLELENSIZE);
	memcpy(newPacket + HANDLE_LABEL_START, target, handleLen);
	fullsend(clientSocket, newPacket, (int) ntohs(packetLen));
}

void send_flag9(int clientSocket) {
	uint16_t packetSize = htons(CHATHEADERSIZE);
	uint8_t flag = FLAG9;
	char newPacket[CHATHEADERSIZE];
	memcpy(newPacket, &packetSize, PDULENSIZE);
	memcpy(newPacket + FLAGPOSITION, &flag, FLAGSIZE);
	fullsend(clientSocket, newPacket, ntohs(packetSize));
}

/* * * * * * * * %L * * * * * * * */
void flag10Case(int clientSocket, linkedList *list) {
	linkedNode *current = list -> head;
	send_flag11(clientSocket, list);
	while (current != NULL) {
		send_flag12(clientSocket, current -> handleName, strlen(current -> handleName));
		current = current->next;
	}
	send_flag13(clientSocket);
}

void send_flag11(int clientSocket, linkedList *list) {
	char newPacket[CHATHEADERSIZE + HANDLERESSIZE];
	uint8_t flag = FLAG11;
	uint32_t numHandles = 0;
	uint16_t packetSize = htons(CHATHEADERSIZE + HANDLERESSIZE);
	linkedNode *current = list -> head;
	while (current != NULL) {
		numHandles++;
		current = current->next;
	}
	numHandles = htonl(numHandles);
	memcpy(newPacket, &packetSize, PDULENSIZE);
	memcpy(newPacket + FLAGPOSITION, &flag, FLAGSIZE);
	memcpy(newPacket + CHATHEADERSIZE, &numHandles, HANDLERESSIZE);
	fullsend(clientSocket, newPacket, ntohs(packetSize));
}

void send_flag12(int clientSocket, char *handle, int len) {
	uint8_t handleLen = len;
	uint8_t flag = FLAG12;
	uint16_t packetSize = htons(CHATHEADERSIZE + HANDLELENSIZE + handleLen);
	char newPacket[MAX_PACKET_SIZE];
	memcpy(newPacket, &packetSize, PDULENSIZE);
	memcpy(newPacket + FLAGPOSITION, &flag, FLAGSIZE);
	memcpy(newPacket + CHATHEADERSIZE, &handleLen, HANDLELENSIZE);
	memcpy(newPacket + CHATHEADERSIZE + HANDLELENSIZE, handle, len);
	fullsend(clientSocket, newPacket, ntohs(packetSize));
}

void send_flag13(int clientSocket) {
	uint8_t flag = FLAG13;
	uint16_t packetSize = htons(CHATHEADERSIZE);
	char newPacket[CHATHEADERSIZE];
	memcpy(newPacket, &packetSize, PDULENSIZE);
	memcpy(newPacket + FLAGPOSITION, &flag, FLAGSIZE);
	fullsend(clientSocket, newPacket, ntohs(packetSize));
}

/* * * * * * * * handle name datastructure functions* * * * * * * */
/* * * * * must dynamically allocate * * * * */
int checkHandleNameExistance(char * handle, linkedList * list) {
	linkedNode *current = list -> head;
	while (current != NULL) {
		if (current -> handleName != NULL && strcmp(current -> handleName, handle) == 0) {
			return -1;
		}
		else{current = current -> next;}
	}
	return 0;
}

void addTolinkedList(linkedList *linkedList, linkedNode *node) {
    if (linkedList -> head == NULL) {linkedList -> head = node;}
    else {
        linkedNode *current = linkedList -> head;
        while (current -> next != NULL) {
            current = current -> next;
        }
        current -> next = node;
    }
}

linkedNode *initializelinkedNode(int socketNum) {
    linkedNode *node = malloc(sizeof(linkedNode));
    if (node == NULL) {
        printf("Error trying to malloc for a LinkedNode\n");
        exit(1);
    }
    else {
    	node -> handleName = NULL;
        node -> socketNum = socketNum;
        node -> next = NULL;
    }
    return node;
}

linkedList *initializelinkedList() {
    linkedList *list = malloc(sizeof(linkedList));
    if (list == NULL) {
        printf("Error tring to malloc for a linkedList\n");
        exit(1);
    }
    else {
    	list -> head = NULL;
    }
    return list;
}

linkedNode *findNode(linkedList *list, int socketNum) {
    linkedNode *current = list -> head;
    while (current != NULL) {
        if (current -> socketNum == socketNum) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void removeHandleFromList(linkedList *list, int socketNum) {
    linkedNode *current = list->head;
    linkedNode *prev = NULL;
    if (current != NULL && current -> socketNum == socketNum) {
        list -> head = current -> next;
        if (current -> handleName != NULL) {
        	free(current->handleName);
	    }
	    free(current);
        return;
    }
    while (current != NULL && current -> socketNum != socketNum) {
        prev = current;
        current = current->next;
    }
    if (current == NULL) {
        return;
    }
    prev->next = current->next;
    if (current -> handleName != NULL) {
    	free(current->handleName);
    }
    free(current);
}




