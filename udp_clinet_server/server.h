#ifndef SERVER_H_ 
#define SERVER_H_

#define MAX_PACKET_SIZE 1024
#define DEBUG_FLAG 1
#define BACKLOG 10
#define xstr(a) str(a)
#define str(a) #a

/* * * * * * sizes * * * * */
#define CHATHEADERSIZE 3
#define PDULENSIZE 2
#define FLAGSIZE 1
#define HANDLELENSIZE 1
#define HANDLERESSIZE 4
#define MAXHANDLE 100
#define MAXTEXTSIZE 200

/* * * * * * position * * * * */
#define FLAGPOSITION 2
#define HANDLELENPOSITION 3 
#define HANDLEPOSITION 4

#define HANDLE_POS 3
#define HANDLE_LABEL_START 4

/* * * * * * flags * * * * */
#define FLAG1 1
#define FLAG2 2
#define FLAG3 3
#define FLAG4 4
#define FLAG5 5
#define FLAG6 6
#define FLAG7 7
#define FLAG8 8
#define FLAG9 9
#define FLAG10 10
#define FLAG11 11
#define FLAG12 12
#define FLAG13 13

/* * * * * * data structure * * * * */
typedef struct linkedNode {
   char *handleName;
   int socketNum;
   struct linkedNode *next;
} linkedNode;

typedef struct linkedList {
   struct linkedNode *head;
} linkedList;


/* * * * * * main * * * * */
int main(int argc, char *argv[]);

/* * * * * * * * connection setup * * * * * * * */
int checkArgs(int argc, char *argv[]);
int tcpServerSetup(int portNumber);

/* * * * * * * * process packets * * * * * * * */
void setAll_and_findHighestNumberFd(linkedNode * head, int highestNumberFd, int serverSocket, fd_set fdvar);
void processPackets(int serverSocket);
void differentiatepackets(int clientSocket, linkedList *list);

/* * * * * * * * flag 1 2 3 * * * * * * * */
void flag1Case(int clientSocket, char * buf, linkedList *list);
int checkHandleNameExistance(char * handle, linkedList * list);

/* * * * * * * * %M * * * * * * * */
void flag5Case(int clientSocket, char * buf, linkedList *list);

/* * * * * * * * %B * * * * * * * */
void flag4Case(int clientSocket, char * buf, linkedList *list);

/* * * * * * * * %L * * * * * * * */
void flag10Case(int clientSocket, linkedList *list);
void send_flag11(int clientSocket, linkedList *list);
void send_flag12(int clientSocket, char *handle, int len);
void send_flag13(int clientSocket);

/* * * * * * * * %E * * * * * * * */
void send_flag7(int clientSocket, char * dest);
void send_flag9(int clientSocket);

/* * * * * * linked list * * * * */
void addTolinkedList(linkedList *linkedList, linkedNode *node);
linkedNode *initializelinkedNode(int socketNum);
linkedList *initializelinkedList();
linkedNode *findNode(linkedList *list, int socketNum);
void removeHandleFromList(linkedList *list, int socketNum);


#endif

