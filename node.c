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
  SETUP_KEY(node);
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
  int *nodeNumbers = SHARED_MEMORY(MAX_NODES*sizeof(int));
  int *temp = nodeNumbers;
  for (int i = 0; i < nodeCount; i++){
    nodeNumbers[i] = nodes[i];
  }
  int *deferredReplies = SHARED_MEMORY(MAX_NODES*sizeof(int));
  temp = deferredReplies;
  for (int i = 0; i < nodeCount; i++){
    deferredReplies[i] = 0;
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

  //Add me protocol
  Message msgAddMe;
  int msgSize = sizeof(Message) - sizeof(long int);

  msgAddMe.msgFrom = node;
  strcpy(msgAddMe.buffer , ADD_ME);
  int i;
  printf("Notifying: ");
  for (i = 0; i < nodeCount; i++){
    msgAddMe.msgTo = (long int)nodes[i];
    if (msgsnd(requestQueue, &msgAddMe, msgSize, 0) == -1){
      perror("Failed to add to protocol");
      return 1;
    }
    printf("%d ", nodes[i]);
  }
  printf("\n");

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
      //child/parent process: cs mutex
      printf("--mutex process up\n");
      Message msgMutex;

      msgMutex.msgTo = node;
      msgMutex.msgFrom = node;
      strcpy(msgMutex.buffer , "HEHE!!\n");

      int randomTime;
      while (1){
        randomTime = rand() %5 + 2;
        sleep(randomTime);
        printf("I'm the mutex and I just woke up!\n");
        if (msgsnd(requestQueue, &msgMutex, msgSize, 0) == -1){
          perror("Mutex failed to send");
        }
        if (msgsnd(replyQueue, &msgMutex, msgSize, 0) == -1){
          perror("Mutex failed to send");
        }

      }
    }

    else {
      //parent process: reply handler
      printf("--reply process up\n");
      Message msgReply;
      while(1){
        //TODO: check for errors
        msgrcv(replyQueue, &msgReply, msgSize, node, 0);
        printf("Reply: %s", msgReply.buffer);
      }
    }
  }

  else {
    //parent process: request handler
    printf("--request process up\n");
    Message msgRequest;
    while(1){
      //TODO: check for errors
      msgrcv(requestQueue, &msgRequest, msgSize, node, 0);
        //Add me request
      if(strcmp(msgRequest.buffer, ADD_ME) == 0){
        //Add to existing nodes
        nodeNumbers[*n] = (int)msgRequest.msgFrom;
        deferredReplies[*n] = 0;
        *n += 1;
        int aux;
        printf("--Nodes are now (%d): ", *n);
        for (aux = 0; aux < *n; aux++) {
          printf("%d ",nodeNumbers[aux]);
        }
        printf("\n");
      }
      else {
        //Actual request message
        printf("Request: %s", msgRequest.buffer);
      }
    }

  }
}
