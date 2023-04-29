void broadcast_to_quorum(int count) {
    int successful_broadcasts = 0;
    for (int i = 0; i < server_num - 1 && successful_broadcasts < count; i++) {
        int socket = createServerSock(1);
        struct broadcast_args *args = (struct broadcast_args *)malloc(sizeof(struct broadcast_args));
        args->arg1 = &socket;
        memcpy(&(args->arg2), &logs, sizeof(logs));

        pthread_t broadcast_thread;
        if (pthread_create(&broadcast_thread, NULL, (void *)broadcast_handler, (void *)args) < 0) {
            perror("Could not create thread");
        } else {
            successful_broadcasts++;
        }
    }
}

void quorum(char *type, char *title, char *content, int N, int NR, int NW) {
    if (strcmp("Post", type) == 0 || strcmp("Reply", type) == 0) {
        if (NW > N / 2) {
            if (is_primary) {
                if (strcmp("Post", type) == 0) {
                    post(time_index++, title, content);
                } else if (strcmp("Reply", type) == 0) {
                    reply(time_index++, title, content);
                }
                // Broadcast to at least NW - 1 other servers
                broadcast_to_quorum(NW - 1);
            } else {
                printf("Not primary, cannot execute write operation\n");
            }
        } else {
            printf("Write quorum not met (NW > N/2), operation cannot be executed\n");
        }
    } else if (strcmp("Choose", type) == 0 || strcmp("Read", type) == 0) {
        if (NR + NW > N) {
            if (is_primary) {
                if (strcmp("Choose", type) == 0) {
                    choose(title);
                } else {
                    int number;
                    scanf("%d", &number);
                    read_list(number);
                }
            } else {
                sleep(2); // Wait for possible propagation from primary
                if (strcmp
   ("Choose", type) == 0) {
                    choose(title);
                } else {
                    int number;
                    scanf("%d", &number);
                    read_list(number);
                }
            }
        } else {
            printf("Quorum not met (NR + NW > N), operation cannot be executed\n");
        }
    } else {
        printf("Invalid operation type\n");
    }
}