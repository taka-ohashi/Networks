#ifndef CCLIENT_H_ 
#define CCLIENT_H_

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
#define HANDLEPOSITION 3
#define HANDLELABELPOSITION 4


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

/* * * * * * main * * * * */
int main(int argc, char *argv[]);

/* * * * * * * * connection setup * * * * * * * */
int tcpClientSetup(char * serverName, char * port, int debugFlag);
void send_flag1(char * clientHandle, int socketNum);
void recv_flag2_or_flag3(int socketNum);
void send_and_receieve_packets(char * clientHandle, int serverSocket);

/* * * * * * * * connection error * * * * * * * */
void checkArgs(int argc, char *argv[]);
void checkHandleName(char *argv[]);
void run_recv_error_check(char * packet);
void recv_flag7(char * packet);

/* * * * * * * * parsing * * * * * * * */
char *parseCommandLine(FILE *file);
void differentiateCommand(char * string, char * handle, int serverSocket);

/* * * * * * * * while(1) * * * * * * * */
void retreiveMessageFromServer(int serverSocket);

/* * * * * * * * %M * * * * * * * */
void send_flag5(int serverSocket, char * handle, char * packet);
void fullsendmessage(int serverSocket, char * handle, char * target, int targetSize, char * packet, int bytesCopied);
void recv_flag5(char * packet);

/* * * * * * * * %B * * * * * * * */
void send_flag4(int serverSocket, char * handle, char * text);
void recv_flag4(char * packet);

/* * * * * * * * %L * * * * * * * */
void send_flag10(int socketNum);
void recv_flag11_12_13(int serverSocket, char * packet);
void print_all_handles(int serverSocket);

/* * * * * * * * %E * * * * * * * */
void send_flag8(int socketNum);
void recv_flag9();


#endif








