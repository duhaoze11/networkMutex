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
  for (int i = 0; i < MAX_NODES; i++){
    nodeNumbers[i] = nodes[i];
  }
  int *deferredReplies = SHARED_MEMORY(MAX_NODES*sizeof(int));
  temp = deferredReplies;
  for (int i = 0; i < MAX_NODES; i++){
    deferredReplies[i] = 0;
  }
  int mutex = SEMAPHORE(SEM_BIN, 1);
  int waitSemaphore = SEMAPHORE(SEM_BIN, 1);

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
      char auxBuffer[10];
      Message msgMutex;
      int randomWait;

      while (1){
        fgets(auxBuffer, 10, stdin);
        printf("I WANT THE CRITICAL SECTION!\n");

        printf("Highest request number is: %d\n", *highestReqNumber);
        P(mutex);
        *requestCS = 1;
        *highestReqNumber += 1;
        *reqNumber = *highestReqNumber;
        V(mutex);
        printf("My request number is: %d\n", *reqNumber);

        *outstandingReplies = *n;

        msgMutex.msgFrom = node;
        int send;
        for (send = 0; send < *n; send++){
          msgMutex.msgTo = nodeNumbers[send];
          sprintf (msgMutex.buffer, "%d", *reqNumber);
          if (msgsnd(requestQueue, &msgMutex, msgSize, 0) == -1){
            perror("Mutex failed to send request");
          }
          printf("Asking %d\n", nodeNumbers[send]);
        }

        while (*outstandingReplies > 0){
          printf("mutex unblocked, outstanding replies: %d\n", *outstandingReplies);
          P(waitSemaphore);
        }

        // BEGINNING OF CRITICAL SECTION //
        msgMutex.msgTo = SERVER;
        msgMutex.msgFrom = node;
        sprintf (msgMutex.buffer, "############## START OUTPUT FOR NODE %d ##############\n", node);
        if (msgsnd(printerQueue, &msgMutex, msgSize, 0) == -1){
          perror("Mutex failed to send");
        }

        strcpy(msgMutex.buffer , "OMG I'M IN THE CRITICAL SECTION!!\n");
        int randomTimes = (rand() % 10) + 1;

        while (randomTimes){
          if (msgsnd(printerQueue, &msgMutex, msgSize, 0) == -1){
            perror("Mutex failed to send");
          }
          sleep(1);
          randomTimes--;
        }

        sprintf (msgMutex.buffer, "-------------- END OUTPUT FOR NODE %d --------------\n", node);
        if (msgsnd(printerQueue, &msgMutex, msgSize, 0) == -1){
          perror("Mutex failed to send");
        }

        // END OF CRITICAL SECTION //

        *requestCS = 0;

        int blocked;
        for (blocked = 0; blocked <= *n; blocked++) {
          if (deferredReplies[nodeNumbers[blocked]]) {
            deferredReplies[nodeNumbers[blocked]] = 0;
            msgMutex.msgTo = nodeNumbers[blocked];
            if (msgsnd(replyQueue, &msgMutex, msgSize, 0) == -1){
              perror("Mutex failed to send reply");
            }
            printf("Replied to %d\n", nodeNumbers[blocked]);
          }
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
        printf("Received a reply from: %d\n", msgReply.msgFrom);
        *outstandingReplies -= 1;
        V(waitSemaphore);
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

      if(strcmp(msgRequest.buffer, ADD_ME) == 0){
        //Add me request
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
        //Actual request message: determine priorities
        int k;
        if (sscanf(msgRequest.buffer, "%d *", &k)){
          printf("Received request with number %d\n", k);
        } else {
          continue;
        }

        int deferIt;
        if (k > *highestReqNumber){
          *highestReqNumber = k;
        }
        P(mutex);
         deferIt = (*requestCS) &&
         ( ( k > *reqNumber) ||
         ( k == *reqNumber && msgRequest.msgFrom > node ) );
        V(mutex);

        if (deferIt){
          //make it wait
          printf("Make %d wait\n", msgRequest.msgFrom);
          deferredReplies[msgRequest.msgFrom] = 1;
        }
        else{
          //send reply
          printf("Cool, you can go %d\n", msgRequest.msgFrom);
          msgRequest.msgTo = msgRequest.msgFrom;
          msgRequest.msgFrom = node;
          if (msgsnd(replyQueue, &msgRequest, msgSize, 0) == -1){
            perror("Mutex failed to send");
          }
        }
      }
    }

  }
}
