#include "tool.h"
#include "server.h"
#define server_num 3
#define time_length 20

int time_array[time_length];
int time_index=0;
int server_socks[5];
int client_socks[5];
int log_count = 0;
struct log_entry logs[10];
bool send_ready,receive_ready;
int is_primary;
int server_sock; //socket to connect primary
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
char messageTosend[1024];
int num_ack = 0;
int option; // 1: primary backup; 2: quorum; 3: local_write
int ID;
int client_port;
int replica_port;
int primary_port;
int primary_ID;
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

// void print_log(int number){
//     printf("log count: %d\n", log_count);
//     pthread_mutex_lock(&log_lock);
//     for(int i=0;i<number;i++){
//         printf("%s\n",logs[i].title);
//         if(logs[i].type){// for non-reply print title and its replies
//             print_reply(logs[i].reply_indexes[i],0);
//         }
//     }
//     pthread_mutex_lock(&log_lock);
// }

void print_log(int count){
    printf("-------------------\n");
    // pthread_mutex_lock(&log_lock);
    int idx;
    for (int i=0;i<count;i++) {
        struct log_entry current = logs[i];
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

// void read_list(int count, char* message){
//     // pthread_mutex_lock(&log_lock);
//     int idx;
//     for (int i=0;i<count;i++) {
//         struct log_entry current = logs[i];
//         char des[50];
//         sprintf(des, "%d - %s\n", current.timestamp, current.title);
//         strcat(message, des);
//         printf("1printmsg %s", message);
//         // print reply
//         for (int j = 0; j < current.reply_count; j++) {
//             // memset(des, 0, sizeof(des));
//             idx = current.reply_indexes[j];
//             strcat(message, "----");
//             printf("printmsg %s", message);
//             // memset(des, 0, sizeof(des));
//             sprintf(des, "%d - %s\n", logs[idx].timestamp, logs[idx].title);
//             strcat(message, des);
//             printf("printmsg %s", message);
//         }
//     }
//     printf("sizeof message: %ld", sizeof(message));
//     // pthread_mutex_lock(&log_lock);
// }

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

/* for the server to write to local copy */
int update_write(char* message) {
    printf("in update write\n");
    char msg_cpy[MAX_MESSAGE_LENGTH];
    strcpy(msg_cpy, message);
    char* token;
    token = strtok(msg_cpy, ";");
    int type;
    int found = 0;
    
    if (strcmp(token, "reply")==0) {
        char* title = strtok(NULL, ";");
        char* reply_title = strtok(NULL, ";");
        char* content = strtok(NULL, ";");
        type = 0;
        pthread_mutex_lock(&log_lock);
        for(int i=0;i<log_count;i++){
            if (strcmp(title,logs[i].title)==0){    //for match title update its reply indexes and post it
                logs[i].reply_indexes[logs[i].reply_count]=log_count;
                logs[i].version++;
                logs[i].reply_count++;
                found = 1;
                break;
            }
        }
        pthread_mutex_unlock(&log_lock);
        if (found == 0) {
            return -1;
        }
        pthread_mutex_lock(&log_lock);
        logs[log_count].timestamp= log_count;
        // title
        logs[log_count].title = malloc(sizeof(reply_title));
        strcpy(logs[log_count].title, reply_title);
        // content
        logs[log_count].content = malloc(sizeof(content));
        strcpy(logs[log_count].content, content);

        logs[log_count].reply_count = 0;
        logs[log_count].type = type;
        log_count++;
        pthread_mutex_unlock(&log_lock);
        printf("update completed\n");
        print_log(log_count);
        return 0;
    }
    else { // is article
        char* title = strtok(NULL, ";");
        char* content = strtok(NULL, ";");
        found = 1;
        type = 1;
        pthread_mutex_lock(&log_lock);
        logs[log_count].timestamp= log_count;
        // title
        logs[log_count].title = malloc(sizeof(title));
        strcpy(logs[log_count].title, title);
        // content
        logs[log_count].content = malloc(sizeof(content));
        strcpy(logs[log_count].content, content);

        logs[log_count].reply_count = 0;
        logs[log_count].type = type;
        log_count++;
        pthread_mutex_unlock(&log_lock);
        printf("update completed\n");
        print_log(log_count);
        printf("back to update write\n");
        return 0;
    }

    printf("upadate failed\n");
    return -1;
}

// void choose(char* message, char* answer){// match titile then print content
//     char* token = strtok(message, ";");
//     char* title = strtok(NULL, ";");
//     printf("inside choose\n");
    
//     // char str[10];
//     // strcpy(str, title);
//     // printf("title is: %s\n", str);
//     strcpy(answer, "Not Found\n");
//     pthread_mutex_lock(&log_lock);
//     for(int i=0;i<log_count;i++){
//         printf("i: %d\n", i);
//         printf("%s\n",logs[i].title);
//         if (strcmp(title,logs[i].title)==0){
//             memset(answer, 0, sizeof(answer));
//             strcpy(answer, logs[i].content);
//             printf("content: %s\n", answer);
//             break;
//         }
//     }
//     pthread_mutex_unlock(&log_lock);
// }

void choose(char* message, char* answer){
    char* token = strtok(message, ";");
    char* title = strtok(NULL, ";");
    printf("inside choose\n");
    
    // Remove any trailing newline characters from the title
    int len = strcspn(title, "\n");
    title[len] = '\0';
    
    strcpy(answer, "Not Found\n");
    pthread_mutex_lock(&log_lock);
    for(int i=0;i<log_count;i++){
        printf("i: %d\n", i);
        printf("%s\n",logs[i].title);
        
        // Remove any trailing newline characters from the log title
        int log_len = strcspn(logs[i].title, "\n");
        logs[i].title[log_len] = '\0';
        
        if (strcmp(title,logs[i].title)==0){
            memset(answer, 0, sizeof(answer));
            strcpy(answer, logs[i].content);
            printf("content: %s\n", answer);
            break;
        }
    }
    pthread_mutex_unlock(&log_lock);
}

/* this is primary used for broadcast for ack */
void broadcast(char* message, char* answer, int* ports, int count){
    // int *ports = listen_primary_ports;

    int sockfd;
    int n;
    int num_acks_received = 0;
    char ack_status[count][MAX_MESSAGE_LENGTH];
    char buf[MAX_MESSAGE_LENGTH];
    memset(ack_status, 0, sizeof(ack_status));

    for (int i = 0; i < count; i++) {
        memset(buf, 0, sizeof(buf));
        sockfd = send_message(IP, ports[i], message);
        receive_udp_message(sockfd, buf);
        
        printf("Received message: %s\n", buf);

        if (strcmp(buf, "ack") == 0) {
            strcpy(ack_status[i], buf);
            num_acks_received++;
        } else {
            strcpy(ack_status[i], "rej");
        }
    }

    char overall_status[MAX_MESSAGE_LENGTH];
    strcpy(overall_status, "ack");
    for (int i = 0; i < count; i++) {
        if (strcmp(ack_status[i], "rej") == 0) {
            strcpy(overall_status, "rej");
            break;
        }
    }
    strcpy(answer, overall_status);
}

// void* replica_listen_request(void* arg){
void* replica_listen_request(){
    replica_port = listen_primary_ports[ID];
    int sock = bind_udp(replica_port);
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        exit(1);
    }
        printf("replica port: %d\n", replica_port);
        printf("replica fd: %d\n", sock);
    
    char buf[MAX_MESSAGE_LENGTH];
    char answer[MAX_MESSAGE_LENGTH];
    while(1) {
        while(is_primary == 1) {
            // printf("not replica!\n");
        }
        struct sockaddr_storage sender_addr;
        socklen_t sender_addrlen = sizeof(struct sockaddr);
        printf("rep waiting for prim on port %d... \n", replica_port);

        recvfrom(sock, buf, MAX_MESSAGE_LENGTH, 0, (struct sockaddr *)&sender_addr, &sender_addrlen);
        printf("replica received request: %s\n", buf);

        /* set priamry request */
        char str[MAX_MESSAGE_LENGTH];
        strcpy(str, buf);
        char* tok;
        tok = strtok(str, ";");
        if (strcmp(tok, "set_primary") == 0) {
            sscanf(buf, "set_primary;%d", &primary_ID);
            is_primary = 0;
            strcpy(answer, "ack");
            printf("Primary set to %d\n", primary_ID);
        
        }
        else {
            printf("before update write\n");
            print_log(log_count);
            /* update request */
            if (update_write(buf) == -1) {
                strcpy(answer, "rej");
            }
            else {
                strcpy(answer, "ack");
            }
        }   
        sendto(sock, answer, MAX_MESSAGE_LENGTH, 0, (struct sockaddr *)&sender_addr, sender_addrlen);
        
    }
}

// void* primary_listen_request(void* arg) {
void* primary_listen_request() {

    // int sock = bind_udp(listen_replica_ports[ID]);
    char buf[MAX_MESSAGE_LENGTH];
    char answer[MAX_MESSAGE_LENGTH];

    primary_port = listen_replica_ports[ID];
    int sock = bind_udp(primary_port);
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        exit(1);
    }
        printf("primary port: %d\n", primary_port);
        printf("primary fd: %d\n", sock);
    

    printf("primary_listen_request thread on port %d\n", primary_port);


    while(1) {
        while(is_primary != 1){}

        // bind to port
        struct sockaddr_storage sender_addr;
        socklen_t addr_len = sizeof(sender_addr);
        int numbytes = recvfrom(sock, buf, MAX_MESSAGE_LENGTH-1 , 0, (struct sockaddr *)&sender_addr, &addr_len);
        if (numbytes == -1) {
            perror("recvfrom");
            exit(1);
        }
        buf[numbytes] = '\0';

        printf("Primary received request: %s\n", buf);
        if (update_write(buf) == -1) {
            strcpy(answer, "rej");
        } else {
            printf("update at primary completed, broadcasting\n");
            int replica_ports[server_num-1];
            int i;
            for (i = 0; i < server_num; i++) {
                if (i < ID) {
                    replica_ports[i] = listen_primary_ports[i];
                }
                else if (i > ID){
                    replica_ports[i-1] = listen_primary_ports[i];
                }
            }
            printf("before broadcast\n");
            broadcast(buf, answer,replica_ports, server_num-1);
        }
        printf("primary send back to rep\n");
        sendto(sock, answer, MAX_MESSAGE_LENGTH, 0, (struct sockaddr *)&sender_addr, addr_len);
    }
}

int broadcast_primary(){
    char m[MAX_MESSAGE_LENGTH];
    char port_str[20];
    sprintf(m, "set_primary;%d", ID);
    char answer[MAX_MESSAGE_LENGTH];
    int replica_ports[server_num-1];
    int i;
    for (i = 0; i < server_num; i++) {
        if (i < ID) {
            replica_ports[i] = listen_primary_ports[i];
        }
        else if (i > ID){
            replica_ports[i-1] = listen_primary_ports[i];
        }
    }
    broadcast(m, answer, replica_ports, server_num-1);
    int is_ack = strcmp(answer, "ack");
    return is_ack;
}


int main(int argc, char *argv[]){

    // port setup
    sscanf(argv[1], "%d", &ID);
    // strategy setup
    printf("strategy? 1 primary_backup/2 quorum/3 local_write\n@> ");
    scanf("%d", &option);

    // primary replica setup
    printf("primary? (1/0)\n@> ");
    scanf("%d", &is_primary);
    printf("is_primary: %d\n", is_primary);
    
    // log setup
    for(int l=0;l<10;l++){ 
        for(int n=0;n<20;n++){
            logs[l].reply_indexes[n]=0;
        }
    }

    // client port bind
    client_port = client_ports[ID];
    int client_fd = bind_udp(client_port);
    if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    printf("client port: %d\n", client_port);
    printf("client fd: %d\n", client_fd);
    
    /* Create threads for listening to server and replica */
    pthread_t primary_thread;
    pthread_t replica_thread;
    // pthread_create(&primary_thread, NULL, (void *)primary_listen_request, &primary_fd);
    // pthread_create(&replica_thread, NULL, (void *)replica_listen_request, &replica_fd);
    pthread_create(&primary_thread, NULL, (void *)primary_listen_request, NULL);
    pthread_create(&replica_thread, NULL, (void *)replica_listen_request, NULL);

    // broadcast primary identity
    if (is_primary == 1) {
        primary_ID = ID;
        if (broadcast_primary() != 0) {
            printf("primary set failed \n");
            exit(1);
        }

    }

    // main loop to accept incomming client request
    char buf[MAX_MESSAGE_LENGTH];
    char message[MAX_MESSAGE_LENGTH];
    char* token;
    while(1){
        // receive from client
        struct sockaddr_storage sender_addr;
        socklen_t addr_len = sizeof(sender_addr);
        int numbytes = recvfrom(client_fd, buf, MAX_MESSAGE_LENGTH-1 , 0, (struct sockaddr *)&sender_addr, &addr_len);
        if (numbytes == -1) {
            perror("recvfrom");
            exit(1);
        }
        buf[numbytes] = '\0';
        printf("Received UDP message: %s\n", buf);

        // server-server communication
        char str[MAX_MESSAGE_LENGTH];
        strcpy(str, buf);
        token = strtok(str, ";");    
        if(strcmp(token,"post")==0 || strcmp(token,"reply") == 0){ // write requests
            if (is_primary){
                if (update_write(buf) == -1) {
                    strcpy(message, "rej");
                }
                else {
                    char answer[MAX_MESSAGE_LENGTH];
                    int replica_ports[server_num-1];
                    int i;
                    for (i = 0; i < server_num; i++) {
                        if (i < ID) {
                            replica_ports[i] = listen_primary_ports[i];
                        }
                        else if (i > ID){
                            replica_ports[i-1] = listen_primary_ports[i];
                        }
                    }
                    broadcast(buf, answer,replica_ports, server_num-1);
                }
            }
            else {
                // send message to primary
                int prim_sock = send_message(IP, listen_replica_ports[primary_ID], buf);
                printf("in the buf: %s\n", buf);
                memset(message, 0, sizeof(buf));
                printf("Contacting primary at port %d...\n", listen_replica_ports[primary_ID]);

                // Wait to get a reply datagram from prime
                recvfrom(prim_sock, message, MAX_MESSAGE_LENGTH, 0, NULL, NULL);
                
            }
        }
        else if (strcmp(token,"read") == 0) {
            memset(message, 0, MAX_MESSAGE_LENGTH);
            read_list(log_count, message);
        }
        else if (strcmp(token,"choose") == 0){
            memset(message, 0, MAX_MESSAGE_LENGTH);
            choose(buf, message);
            printf("after choose\n");
        }

        // send message back to client
        int n = sendto(client_fd, message, strlen(message) + 1, 0, (struct sockaddr *)&sender_addr, addr_len);
        printf("Answer: %d bytes\n", n);

    }

    pthread_join(replica_thread, NULL);
    pthread_join(primary_thread, NULL);
    return 0;

}