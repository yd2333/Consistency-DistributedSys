#include "tool.h"
#include "server.h"


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
                log_version ++;
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
        logs[log_count].version = 0;
        logs[log_count].type = type;
        log_count++;
        log_version++;
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

        log_version++;
        return 0;
    }

    printf("upadate failed\n");
    return -1;
}

void choose(char* message, char* answer){
    char* token = strtok(message, ";");
    char* title = strtok(NULL, ";");
    
    // Remove any trailing newline characters from the title
    int len = strcspn(title, "\n");
    title[len] = '\0';
    strcpy(answer, "Not Found\n");
    pthread_mutex_lock(&log_lock);
    for(int i=0;i<log_count;i++){
        
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
        if (ports[i] == 0) {
            continue;
        }
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

int find_version_wiz_title(char* title){
    
    // Remove any trailing newline characters from the title
    int len = strcspn(title, "\n");
    title[len] = '\0';
    pthread_mutex_lock(&log_lock);
    for(int i=0;i<log_count;i++){
        
        // Remove any trailing newline characters from the log title
        int log_len = strcspn(logs[i].title, "\n");
        logs[i].title[log_len] = '\0';
        
        if (strcmp(title,logs[i].title)==0){
            return logs[i].version;
        }
    }
    pthread_mutex_unlock(&log_lock);
    return -1;
}

// read quorum version broadcast
// find the var value with the largest version
// var: the copy we are looking for
void read_with_vote(char* var, char* answer){
    int sockfd;
    int n;
    int num_acks_received = 0;
    int versions[nr];
    char buf[MAX_MESSAGE_LENGTH];

    // find the readquorum port
    // int servers[nr];
    // for (int i = 0; i < nr; i++) {
    //     if (read_idx[i] == ID) {
    //         servers[i] = 0;
    //         continue;
    //     }
    //     servers[i] = listen_primary_ports[read_idx[i]];
    // }
    char vote_msg[MAX_MESSAGE_LENGTH] = "version;";
    strcat(vote_msg, var);
    int latest_version = -1;
    int current_version;
    char latest_content[MAX_MESSAGE_LENGTH];
    char current_content[MAX_MESSAGE_LENGTH];
    int ports[nr];
    printf("Searching the variable with largest version num ...\n");
    for (int i = 0; i < nr; i++) {
        printf("Ready to ask for vernum to server%d at port %d\n", read_idx[i], listen_primary_ports[read_idx[i]]);

        if (read_idx[i] == ID) {
            if (strcmp(var, "logs") == 0) {
                current_version = log_version;
            }
            else {
                current_version = find_version_wiz_title(var);
            }
        } else {
            memset(buf, 0, sizeof(buf));
            printf("Readty to version request to server%d at port %d\n", read_idx[i], listen_primary_ports[read_idx[i]]);
            sockfd = send_message(IP, listen_primary_ports[read_idx[i]], vote_msg);
            printf("Sent version request to server%d at port %d\n",read_idx[i], listen_primary_ports[read_idx[i]]);
            receive_udp_message(sockfd, buf);
            printf("buf: %s", buf);

            // parse buf
            memset(current_content, 0, sizeof(current_content));
            char str[sizeof(buf)];
            strcpy(str, buf);
            char*  tok;
            tok = strtok(str, ";");
            current_version = atoi(tok);
            printf("%d\n", current_version);
            tok = strtok(NULL, ";");
            strcpy(current_content, tok);
            // sscanf(buf, "%d;%s", &current_version, current_content);
            // current_version = atoi(buf[0]);
            
            printf("data loaded to buf\n");
        }
        // update latest
        if (current_version > latest_version) {
            strcpy(latest_content, current_content);
            printf("after copy\n");
        }
    }
    strcpy(answer, latest_content);
    printf("after copy2\n");

}

void* broadcast_threading(void* input){
    struct broadcast_args *arg = (struct broadcast_args*)input;
    broadcast(arg->arg1, arg->result, arg->ports, arg->port_count);
    free(arg);
}

int check_avalability(char* message) {
    printf("in check\n");
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
                found = 1;
                break;
            }
        }
        pthread_mutex_unlock(&log_lock);
        if (found == 0) {
            return -1;
        } else {return 0;}
        
    }
    return 0;
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
    char answer[MAX_MESSAGE_LENGTH+20];
    while(1) {
        while(is_primary == 1) {
            // printf("not replica!\n");
        }
        struct sockaddr_storage sender_addr;
        socklen_t sender_addrlen = sizeof(struct sockaddr);
        printf("rep waiting for prim on port %d... \n", replica_port);
        memset(buf, 0, sizeof(buf));
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
        
        else if (strcmp(tok, "vote") == 0) {
            memset(answer, 0, sizeof(answer));
            if (check_avalability(buf) == -1) {
                strcpy(answer, "rej");
            } else {strcpy(answer, "ack");}
        }

        else if (strcmp(tok, "version") == 0) {
            tok = strtok(NULL, ";");
            int version;
            if (strcmp(tok, "logs") == 0){
                memset(answer, 0, sizeof(answer));
                char content[MAX_MESSAGE_LENGTH-20];
                read_list(log_count, content);
                version = log_version;
                sprintf(answer, "%d;%s", version, content);
                printf("Log version prepared:\n %s\n", answer);
            }
            else{
                printf("inside else\n");
                memset(answer, 0, sizeof(answer));
                version = find_version_wiz_title(tok);
                printf("v:%d\n", version);
                char message[MAX_MESSAGE_LENGTH];
                char content[MAX_MESSAGE_LENGTH+15];
                memset(content, 0, sizeof(content));
                printf("strcat tok: %s\n", tok);
                sprintf(message, "choose;%s", tok);
                printf("messg: %s\n", message);
                choose(message, content);
                printf("content: %s\n", content);
                sprintf(answer, "%d;%s", version, content);
                printf("Version prepared: %s\n", answer);
            }
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

// This is for the write request
int ask_quorum_vote(char* request) {
    // quorum servers
    int servers[nw];
    for (int i = 0; i < nw; i++) {
        if (write_idx[i] == ID) {
            servers[i] = 0;
            continue;
        }
        servers[i] = listen_primary_ports[write_idx[i]];
    }
    char vote_msg[MAX_MESSAGE_LENGTH] = "vote;";
    strcat(vote_msg, request);
    char answer[MAX_MESSAGE_LENGTH];
    memset(answer, 0, sizeof(answer));
    printf("Primary broadcast for latest version number\n");
    broadcast(vote_msg, answer, servers, nw);
    if (strcmp(answer, "ack")) {
        printf("vote yes\n");
        return 1;
    }
    return -1;

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
        
        char buf_copy[MAX_MESSAGE_LENGTH];
        char* token;
        strcpy(buf_copy, buf);
        token = strtok(buf_copy, ";");

        // case 1: migrate request
        if (strcmp(token, "copy") == 0) {
            int n;
            // 1 memcpy the whole struct
            char encoded[ MAX_LOG_LENGTH*sizeof(struct log_entry) +1];
            memcpy(encoded, logs, sizeof(logs));
            encoded[MAX_LOG_LENGTH*sizeof(struct log_entry)] = '\0';
            n = sendto(sock, encoded, MAX_LOG_LENGTH*sizeof(struct log_entry) +1, 0, (struct sockaddr *)&sender_addr, addr_len);
            // 1 Migrate one by one
            // for (int i = 0;i < log_count; i++){
            //     n = sendto(sock, &logs[i], sizeof(struct log_entry), 0, (struct sockaddr *)&sender_addr, addr_len);
            //     printf("Copy migrated: %d bytes\n", n);
            // }
            // memset(answer, 0, sizeof(answer));
            // strcpy(answer, "end");
            // n = sendto(sock, answer, sizeof(answer), 0, (struct sockaddr *)&sender_addr, addr_len);
            // printf("Migrated end\n");

            // 2 as struct
            // n = sendto(sock, &logs, sizeof(logs) +1, 0, (struct sockaddr *)&sender_addr, addr_len);
            // print_log(log_count);
            // printf("Migrated end: %d\n", n);
        }

        // case 2: set_primary_request
        else if (strcmp(token, "set_primary") == 0) {
            sscanf(buf, "set_primary;%d", &primary_ID);
            is_primary = 0;
            strcpy(answer, "ack");
            printf("Primary set to %d\n", primary_ID);
            sendto(sock, answer, MAX_MESSAGE_LENGTH, 0, (struct sockaddr *)&sender_addr, addr_len);
        }

        // case 3: write request reply/post
        else if(strcmp(token, "reply") == 0 || strcmp(token, "post") == 0) {
            // if is quorum, have to aquire permission first
            if (option == 3){
                if (ask_quorum_vote(buf) == -1) {
                    memset(answer, 0, sizeof(answer));
                    strcpy(answer, "rej");
                    sendto(sock, answer, MAX_MESSAGE_LENGTH, 0, (struct sockaddr *)&sender_addr, addr_len);
                    continue;
                }
            }
            // update local copy of primary itself
            if (update_write(buf) == -1) {
                strcpy(answer, "rej");
            } else {
                // broadcasting
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
        // case 4: read
        else if(strcmp(token, "read") == 0) { // only quorum send remote read request
            read_with_vote("logs", answer);
            sendto(sock, answer, MAX_MESSAGE_LENGTH, 0, (struct sockaddr *)&sender_addr, addr_len);
        }
        else if(strcmp(token, "choose") == 0) { // only quorum send remote read request
            token = strtok(NULL, ";");
            read_with_vote(token, answer);
            sendto(sock, answer, MAX_MESSAGE_LENGTH, 0, (struct sockaddr *)&sender_addr, addr_len);

        }
    }
}

// broadcast the primary identity of self
int broadcast_primary(){
    char m[MAX_MESSAGE_LENGTH];
    char port_str[20];
    sprintf(m, "set_primary;%d", ID);
    char answer[MAX_MESSAGE_LENGTH];
    int replica_ports[server_num];
    for (int i = 0; i < server_num; i++) {
        if (i == ID) {
            replica_ports[i]== 0;
        }
        else if (i == primary_ID){
            replica_ports[i] = listen_replica_ports[i];
        }
        else {
            replica_ports[i] = listen_primary_ports[i];
        }
    }
    broadcast(m, answer, replica_ports, server_num);
    int is_ack = strcmp(answer, "ack");
    return is_ack;
}


/* 
 * handle incoming request with priamary backup
 * request: request from client
 * answer: store the answer to client
 */
void primary_backup_write(char* request, char* answer) {
    if (is_primary){
        if (update_write(request) == -1) {
            strcpy(answer, "rej");
        }
        else {
            // broadcast setups
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
            broadcast(request, answer,replica_ports, server_num-1);
        }
    }
    else {
        // send message to primary
        int prim_sock = send_message(IP, listen_replica_ports[primary_ID], request);
        printf("in the buf: %s\n", request);
        memset(answer, 0, sizeof(answer));
        printf("Contacting primary at port %d...\n", listen_replica_ports[primary_ID]);

        // Wait to get a reply datagram from prime
        recvfrom(prim_sock, answer, MAX_MESSAGE_LENGTH, 0, NULL, NULL);      
    }

}

/* 
 * handle incoming request with local write
 * request: request from client
 * answer: store the answer to client
 */
void local_write(char* request, char* answer) {
    // ask for primary copy of log
    if (is_primary == 0) {
        char copy_request[40] = "copy";
        int sock = send_message(IP, listen_replica_ports[primary_ID], copy_request);
        // clear logs
        memset(logs, 0, sizeof(logs));
        // 0 parsing message
        int n;
        char message[MAX_LOG_LENGTH*MAX_MESSAGE_LENGTH + 1];
        n = recvfrom(sock, message, (MAX_LOG_LENGTH*MAX_MESSAGE_LENGTH + 1), 0, NULL, NULL);
        for (int i = 0; i < MAX_LOG_LENGTH; i++) {
            memcpy(&logs[i], &message[i * sizeof(struct log_entry)], sizeof(struct log_entry));
            printf("log%d %s\n", i, logs[i].title);
        }
        // // 1 keep receiving till "end" is received
        // char message[sizeof(struct log_entry)];
        // for (int i = 0; i < MAX_LOG_LENGTH; i++) {
        //     memset(message, 0, sizeof(message));
        //     recvfrom(sock, message, sizeof(struct log_entry), 0, NULL, NULL);
        //     if (strcmp(message, "end") == 0) {
        //         break;
        //     }
        //     memcpy(&logs[i], message, sizeof(struct log_entry));
        //     log_count++;
        //     print_log(log_count);
        // }

        // 2 receive as struct
        // int n;
        // printf("before received copy\n");
        // print_log(1);

        // n = recvfrom(sock, &logs, sizeof(logs) + 1, 0, NULL, NULL);
        // printf("received %d bytes\n", n);
        // print_log(1);

        // char message[MAX_LOG_LENGTH*sizeof(struct log_entry)];
        // // Decode the string back to an array of structs
        // memset(logs, 0, sizeof(logs));
        // printf("after memset logs\n");
        // int bytes_received = recvfrom(sock, logs, sizeof(logs), 0, NULL, NULL);
        // printf("Received log copy: %d\n", bytes_received);
        // print_log(log_count);

        
        
        // broadcast pirme identity
        if (broadcast_primary()== 0) {is_primary = 1;}
    }

    // update write
    if (update_write(request) == -1) {
        strcpy(answer, "update failed\n");
        return;
    }
    
    // broadcast_args set
    pthread_t broadcast_thread;
    struct broadcast_args* arg = malloc(sizeof(struct broadcast_args));
    strcpy(arg->arg1,request);
    strcpy(arg->result,answer);
    arg->port_count = server_num-1;

    int i;
    for (i = 0; i < server_num; i++) {
        if (i < ID) {
            arg->ports[i] = listen_primary_ports[i];
        }
        else if (i > ID){
            arg->ports[i-1] = listen_primary_ports[i];
        }
    }
    pthread_create(&broadcast_thread, NULL, (void*) broadcast_threading, (void*) arg);
    strcpy(answer, "update success (broadcast on backend...)\n");
}

void local_read(char* answer) {
    memset(answer, 0, MAX_MESSAGE_LENGTH);
    read_list(log_count, answer);
}

void local_choose(char* request, char* answer) {
    memset(answer, 0, MAX_MESSAGE_LENGTH);
    choose(request, answer);
}

void quorum_write(char* request, char* answer) {
    if (is_primary){
        if (ask_quorum_vote(request) == -1){
            strcpy(answer, "rej");
        }

        else if (update_write(request) == -1) {
            strcpy(answer, "rej");
        }
        else {
            // broadcast setups
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
            broadcast(request, answer,replica_ports, server_num-1);
        }
    }
    else {
        // send message to primary
        int prim_sock = send_message(IP, listen_replica_ports[primary_ID], request);
        printf("in the buf: %s\n", request);
        memset(answer, 0, sizeof(answer));
        printf("Contacting primary at port %d...\n", listen_replica_ports[primary_ID]);

        // Wait to get a reply datagram from prime
        recvfrom(prim_sock, answer, MAX_MESSAGE_LENGTH, 0, NULL, NULL);      
    }
}

void quorum_read(char*request, char* answer) {
    memset(answer, 0, MAX_MESSAGE_LENGTH);
    int sock = send_message(IP, listen_replica_ports[primary_ID], request);
    receive_udp_message(sock, answer);
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
    memset(logs, 0, sizeof(logs));
    // for(int l=0;l<MAX_LOG_LENGTH;l++){ 
    //     for(int n=0;n<MAX_LOG_LENGTH;n++){
    //         logs[l].reply_indexes[n]=0;
    //     }
    // }

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
    pthread_t broadcast_thread;
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
            // set quorum parameter
        if (option == 2) {
            printf("nw nr?\n@> ");
            scanf("%d %d", &nw, &nr);
            if (is_primary){
                pick_n_idx(3, nr, read_idx);
                pick_n_idx(3, nw, write_idx);
            }
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
            if (option == 1) {
                primary_backup_write(buf, message);
            }
            else if (option == 2) {
                quorum_write(buf, message);
            }
            else if (option == 3) {
                local_write(buf, message);
            }
        }
        else if (strcmp(token,"read") == 0) {
            if (option == 1 || option == 3) {
                local_read(message);
            }
            if (option == 2) {
                quorum_read(buf, message);
            }
        }
        else if (strcmp(token,"choose") == 0){
            if (option == 1 || option == 3) {
                local_choose(buf, message);
            }
            if (option == 2) {
                quorum_read(buf, message);
            }
        }

        // send message back to client
        int n = sendto(client_fd, message, strlen(message) + 1, 0, (struct sockaddr *)&sender_addr, addr_len);
        printf("Answer: %d bytes\n", n);

    }

    pthread_join(replica_thread, NULL);
    pthread_join(primary_thread, NULL);
    return 0;

}