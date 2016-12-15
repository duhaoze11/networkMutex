#include "serverMsg.h"

int main (){
  char buffer[BUFFER_SIZE];
  int printerQueue, replyQueue, requestQueue, n;
  Message msg;

  key_t printerTok = ftok(".", PRINTER);
  key_t requestTok = ftok(".", REQUEST);
  key_t replyTok = ftok(".", REPLY);

  if ((printerQueue = msgget(printerTok, IPC_CREAT | 0660)) == -1){
    perror("Queue creation failed");
    return 1;
  }
  if ((requestQueue = msgget(requestTok, IPC_CREAT | 0660)) == -1){
    perror("Queue creation failed");
    return 1;
  }
  if ((replyQueue = msgget(replyTok, IPC_CREAT | 0660)) == -1){
    perror("Queue creation failed");
    return 1;
  }

  printf("Printing server is up! Queue: %d\n", printerQueue);

  int msgSize = sizeof(Message) - sizeof(long int);

  while(1){
    n = msgrcv(printerQueue, &msg, msgSize, SERVER, 0);
    printf("%s", msg.buffer);
  }

  return 1;
}
