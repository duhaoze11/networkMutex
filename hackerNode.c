#include "serverMsg.h"
#include <unistd.h>

int main(){
  int printerQueue;

  key_t printerTok = ftok(".", PRINTER);

  if ((printerQueue = msgget(printerTok, IPC_CREAT | 0660)) == -1){
    perror("Queue creation failed");
    return 1;
  }

  Message msg;
  msg.msgTo = SERVER;
  msg.msgFrom = getpid();
  strcpy(msg.buffer , "BUAHAHAHA!!\n");

  printf("Hacker node started! Sending to queue: %d\n", printerQueue);

  int msgSize = sizeof(Message) - sizeof(long int);
  int randomTime;

  while (1){
    randomTime = rand() %5 + 1;
    sleep(randomTime);
    printf("Hacking!\n");
    if (msgsnd(printerQueue, &msg, msgSize, 0) == -1){
      perror("Hacker failed to send");
    }
  }
}
