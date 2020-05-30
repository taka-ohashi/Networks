#ifndef SHARED_H_ 
#define SHARED_H_

void fullsend(int socketNum, char * packet, int len);
void fullrecv(int socketNum, char * packet, int * messageLen); 

#endif


