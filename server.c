#include "tool.h"
#include "server.h"
#define server_num 5
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
int primary_ID = 0;
int yes = 1;

/* for the server to write to local copy */
int update_write(char* message) {
    char* token;
    token = strtok(message, ";");
    char* title = strtok(NULL, ";");
    char* content = strtok(NULL, ";");
    int found = 1;
    if (strcmp(token, "reply")==0) {
        found = 0;
        pthread_mutex_lock(&log_lock);
        char reply_title[30];
        for(int i=0;i<log_count;i++){
            if (strcmp(title,logs[i].title)==0){    //for match title update its reply indexes and post it
                char str[30];
                logs[i].reply_indexes[logs[i].reply_count]=log_count;
                logs[i].version++;
                found = 1;
                break;
            }
        }
        pthread_mutex_unlock(&log_lock);
        }
        
        if (found == 1) {
            pthread_mutex_lock(&log_lock);
            logs[log_count].timestamp= log_count;
            logs[log_count].title = title;
            logs[log_count].content= content;
            logs[log_count].reply_count = 0;
            logs[log_count].reply_indexes;
            log_count++;
            pthread_mutex_unlock(&log_lock);
            return 0;
        }
    
    return 1;
}

void print_reply(int index,int loop){//recursively print every reply
    while(loop>0){
        printf("    ");
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

void read_list(int number){
    pthread_mutex_lock(&log_lock);
    for(int i,j=0;i<number;i++){
        printf("%s\n",logs[i].title);
        if(logs[i].type){// for non-reply print title and its replies
            printf("%s\n",logs[i].title);
            print_reply(logs[i].reply_indexes[0],1);
        }
    }
    pthread_mutex_lock(&log_lock);
}

void choose(char* title){// match titile then print content
    pthread_mutex_lock(&log_lock);
    for(int i=0;i<10;i++){
        if (strcmp(title,logs[i].title)==0){
            printf("%s\n",logs[i].content);
            break;
        }
    }
    pthread_mutex_lock(&log_lock);
}

/* this is primary used for broadcast for ack */
void broadcast(char* message, char* answer, int* ports){
    // int *ports = listen_primary_ports;

    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(servaddr.sin_addr));

    int num_acks_received = 0;
    char ack_status[5][MAX_MESSAGE_LENGTH];
    memset(ack_status, 0, sizeof(ack_status));

    for (int i = 0; i < 5; i++) {
        servaddr.sin_port = htons(ports[i]);
        sendto(sockfd, message, strlen(message), 0, (const struct sockaddr*)&servaddr, sizeof(servaddr));
        

        char buf[MAX_MESSAGE_LENGTH];
        socklen_t len = sizeof(servaddr);
        int n =recvfrom(sockfd, buf, MAX_MESSAGE_LENGTH, MSG_WAITALL, (struct sockaddr*)&servaddr, &len);
        if (n < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }
        buf[n] = '\0';
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
    for (int i = 0; i < 5; i++) {
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
        struct sockaddr sender_addr;
        socklen_t sender_addrlen = sizeof(struct sockaddr);
        printf("rep waiting for prim on port %d... \n", replica_port);

        recvfrom(sock, buf, MAX_MESSAGE_LENGTH, 0, &sender_addr, &sender_addrlen);
        printf("replica received request: %s", buf);

        if (update_write(buf) == 1) {
            strcpy(answer, "rej");
        }
        sendto(sock, answer, MAX_MESSAGE_LENGTH, 0, &sender_addr, sender_addrlen);
        
    }
}

// void* primary_listen_request(void* arg) {
void* primary_listen_request() {

    // bind to port
    primary_port = listen_replica_ports[ID];
    int sock = bind_udp(primary_port);
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        exit(1);
    }
        printf("primary port: %d\n", primary_port);
        printf("primary fd: %d\n", sock);
    

    printf("primary_listen_request thread on port %d\n", primary_port);
    // int sock = bind_udp(listen_replica_ports[ID]);
    char buf[MAX_MESSAGE_LENGTH];
    char answer[MAX_MESSAGE_LENGTH];

    while(1) {
        while(is_primary != 1){}
        struct sockaddr sender_addr;
        socklen_t sender_addrlen = sizeof(struct sockaddr);
        printf("primary waiting for rep ... \n");
        recvfrom(sock, buf, MAX_MESSAGE_LENGTH, 0, &sender_addr, &sender_addrlen);
        printf("primary received request: %s", buf);
        if (update_write(buf) == 1) {
            strcpy(answer, "rej");
        } else {
            broadcast(buf, answer,listen_replica_ports);
        }
        sendto(sock, answer, MAX_MESSAGE_LENGTH, 0, &sender_addr, sender_addrlen);
    }
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

    // // bind ports
    // // int client_fd = bind_udp(client_ports[ID]);
    // int client_fd = bind_udp_dynamic(&client_port);
    // int yes = 1;
    // if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    //     perror("setsockopt");
    //     exit(1);
    // }
    // printf("client port: %d\n", client_port);

    // printf("client port assigned: %d\n", client_port);
    // int primary_fd = bind_udp_dynamic(&primary_port);
    // if (setsockopt(primary_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    //     perror("setsockopt");
    //     exit(1);
    // }
    //     printf("primary port: %d\n", primary_port);

    // int replica_fd = bind_udp_dynamic(&replica_port);
    // if (setsockopt(replica_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    //     perror("setsockopt");
    //     exit(1);
    // }
    //     printf("replica port: %d\n", replica_port);
    // return 0; 
    // bind ports
    // int client_fd = bind_udp(client_ports[ID]);
    client_port = client_ports[ID];
    int client_fd = bind_udp(client_port);
    if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    printf("client port: %d\n", client_port);
        printf("client fd: %d\n", client_fd);


    // primary_port = listen_replica_ports[ID];
    // int primary_fd = bind_udp(primary_port);
    // if (setsockopt(primary_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    //     perror("setsockopt");
    //     exit(1);
    // }
    //     printf("primary port: %d\n", primary_port);
    //     printf("primary fd: %d\n", primary_fd);

    // replica_port = listen_primary_ports[ID];
    // int replica_fd = bind_udp(replica_port);
    // if (setsockopt(replica_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    //     perror("setsockopt");
    //     exit(1);
    // }
    //     printf("replica port: %d\n", replica_port);
    //     printf("replica fd: %d\n", replica_fd);


    
    /* Create threads for listening to server and replica */
    pthread_t primary_thread;
    pthread_t replica_thread;
    // pthread_create(&primary_thread, NULL, (void *)primary_listen_request, &primary_fd);
    // pthread_create(&replica_thread, NULL, (void *)replica_listen_request, &replica_fd);
    pthread_create(&primary_thread, NULL, (void *)primary_listen_request, NULL);
    pthread_create(&replica_thread, NULL, (void *)replica_listen_request, NULL);



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
                char answer[MAX_MESSAGE_LENGTH];
                broadcast(message, answer, listen_primary_ports);
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
        // else if (strcmp(func,"read") == 0) {}
        // else if (strcmp(func,"choose") == 0){}

        // send message back to client
        sendto(client_fd, message, strlen(buf) + 1, 0, (struct sockaddr *)&sender_addr, addr_len);

    }

    pthread_join(replica_thread, NULL);
    pthread_join(primary_thread, NULL);
    return 0;

}