#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>


struct log_entry {
    int version;
    int timestamp;
    int type;              //1 for article,0 for reply
    char* title;
    char* content;
    int reply_count;
    int reply_indexes[20]; //array of reply message indexes
};

struct arg_struct {
    int*  arg1;
    char arg2[10];
};

struct broadcast_args {
    int*  arg1; // message
    char result[MAX_MESSAGE_LENGTH];
};