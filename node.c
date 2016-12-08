#include "serverMsg.h"
#include "ezipc.h"

#define MAX_NODES 100
#define NODE_FILE ".nodes"

void initializedSharedVariables(int nodeCount);

int main(int argc, char *argv[]){
  if (argc != 2){
    printf("Node requires id as parameter to run\n");
    return 1;
  }

  int node = atoi(argv[1]);
  printf("I'm node %d :)\n", node);

  FILE *nodeFile = fopen(NODE_FILE, "a+");
  rewind(nodeFile);

  int nodeCount = 0;
  int nodes[MAX_NODES];
  while(fscanf (nodeFile, "%d *", &nodes[nodeCount]) == 1 && nodeCount < MAX_NODES){
    nodeCount++;
  }

  fprintf(nodeFile, "%d\n", node);
  fclose(nodeFile);

  //Initializing shared variables
  SETUP();
  int *n = SHARED_MEMORY(sizeof(int));
  *n = nodeCount;
  int *reqNumber = SHARED_MEMORY(sizeof(int));
  *reqNumber = 0;
  int *highestReqNumber = SHARED_MEMORY(sizeof(int));
  *highestReqNumber = 0;
  int *outstandingReplies = SHARED_MEMORY(sizeof(int));
  *outstandingReplies = 0;
  int *requestCS = SHARED_MEMORY(sizeof(int));
  *requestCS = 0;
  int *deferredReplies = SHARED_MEMORY(MAX_NODES*sizeof(int));
  int *temp = deferredReplies;
  for (int i = 0; i < nodeCount; i++){
    // *temp = 0;
    // (*temp)++;
    temp[i] = 0;
  }
  int mutex = SEMAPHORE(SEM_CNT, 1);
  int waitSemaphore = SEMAPHORE(SEM_CNT, 1);

  //Initializing message queues
  int printerQueue, replyQueue, requestQueue;

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

  //Forking to create three processes
  int pid = fork();
  if (pid == -1){
    perror("Fork failed D:");
    return 2;
  }
  else if (pid == 0) {
    //child process: reply handler
    pid = fork();
    if (pid == -1){
      perror("Fork failed D:");
      return 2;
    }
    else if (pid == 0) {
      //child process: cs mutex
      printf("I'M THE MUTEX PROCESS :D\n");

    }
    else {
      //parent process: reply handler
      printf("I'M THE REPLY PROCESS :D\n");
    }
  }
  else {
    //parent process: request handler
    printf("I'M THE REQUEST PROCESS :D\n");
    // while(1){
    //   n = msgrcv(requestQueue, &msg, msgSize, 1, 0);
    //   printf("%s", msg.buffer);
    // }

  }
}
