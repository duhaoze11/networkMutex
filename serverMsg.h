#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 500

const char PRINTER = 'M';
const char REQUEST = 'E';
const char REPLY = 'L';
const long int SERVER = 1L;
typedef struct {
  long int msgTo;
  long int msgFrom;
  char buffer[BUFFER_SIZE];
} Message;
