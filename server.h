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

#define MAX_LOG_LENGTH 10
#define server_num 3
#define time_length 20

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
    char arg1[MAX_MESSAGE_LENGTH]; // message
    char result[MAX_MESSAGE_LENGTH]; // to store result
    int ports[10]; //ports to free
    int port_count; // number of port
};




// log param
struct log_entry logs[MAX_LOG_LENGTH];
int log_count = 0;
int log_version = 0; // each reply is 2 update

// server self config
int is_primary;
int option; // 1: primary backup; 2: quorum; 3: local_write
int ID;
int client_port;
int replica_port;
int primary_port;
int primary_ID;

// quorum
int nw;
int nr;
int read_idx[server_num];
int write_idx[server_num];

//multi thread
int server_sock; //socket to connect primary
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
int yes = 1;

void print_reply(int index,int loop){ //recursively print every reply
    while(loop>0){
        printf("----");
        loop--;
    }
    printf("%s\n",logs[index].title);
    if(logs[index].reply_indexes[0]){
        int i=0;
        while(logs[index].reply_indexes[i]){
            print_reply(logs[index].reply_indexes[i],loop+1);
            i++;
        }
    }
    else{
        return;
    }

}

void print_log(int count){
    printf("-------------------\n");
    // pthread_mutex_lock(&log_lock);
    int idx;
    for (int i=0;i<count;i++) {
        printf("----1\n");
        struct log_entry current = (struct log_entry)logs[i];
        printf("----3\n");

        printf("----2\n");
        if (current.type == 0) {
            continue;
        }
        printf("%d - %s - %s\n", current.timestamp, current.title, current.content);
        // print reply
        for (int j = 0; j < current.reply_count; j++) {
            idx = current.reply_indexes[j];
            printf("----");
            printf("%d - %s - %s\n", logs[idx].timestamp, logs[idx].title, logs[idx].content);
        }
    }
    printf("-------------------\n");
    // pthread_mutex_lock(&log_lock);
}

void read_list(int count, char* message){
    int idx;
    for (int i=0;i<count;i++) {
        struct log_entry current = logs[i];
        if (current.type == 0) {
            continue;
        }
        char des[50];
        sprintf(des, "%d - %s\n", current.timestamp, current.title);
        strcat(message, des);
        // print reply
        for (int j = 0; j < current.reply_count; j++) {
            idx = current.reply_indexes[j];
            strcat(message, "----");
            sprintf(des, "%d - %s\n", logs[idx].timestamp, logs[idx].title);
            strcat(message, des);
        }
    }
    printf("Length of message: %ld", strlen(message));
}