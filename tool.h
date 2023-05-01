#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
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

#define MAX_KEY_LENGTH 256
#define MAX_VALUE_LENGTH 1024
#define MAX_MESSAGE_LENGTH 1280


char IP[20]="127.0.0.1";
int client_ports[] = {51599,30883,45693,40404,50612};
int listen_replica_ports[] = {48104,47932,36046,61099,57165};
int listen_primary_ports[] = {34154,33132,51543,32438,43974};

// server bind to port
// int bind_udp(int port){
// int bind_udp(int port){
//     int sockfd;
//     struct sockaddr_in cli_addr;

//     if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
//         printf("\n Socket creation error \n");
//         return -1;
//     }

//     memset(&cli_addr, '0', sizeof(cli_addr));

//     /* Hardcoded IP and Port for every client*/
//     cli_addr.sin_family = AF_INET;
//     cli_addr.sin_port = htons(port);
//     cli_addr.sin_addr.s_addr = inet_addr(IP);

//     /* Bind the socket to a specific port */
//     if (bind(sockfd, (const struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0) {
//         printf("\nBind failed\n");
//         return -1;
//     }

//     return sockfd;
// }

int bind_udp(int port){
    int sockfd;
    struct sockaddr_in cli_addr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&cli_addr, '0', sizeof(cli_addr));

    /* Hardcoded IP and Port for every client*/
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(port);
    cli_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* Bind the socket to a specific port */
    if (bind(sockfd, (const struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0) {
        printf("\nBind failed\n");
        return -1;
    }

    /* Set IP and Port */
    struct sockaddr_in tmp_addr;
    socklen_t len = sizeof(tmp_addr);
    getsockname(sockfd, (struct sockaddr *) &tmp_addr, &len);
    // client_IP = inet_ntoa(tmp_addr.sin_addr);
    // client_Port = ntohs(tmp_addr.sin_port);
    
    // printf("Client IP: %s, Port: %d\n", client_IP, client_Port);
    return sockfd;
}

int bind_udp_dynamic(int* port){
    int sockfd;
    struct sockaddr_in cli_addr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&cli_addr, '0', sizeof(cli_addr));

    /* Hardcoded IP and Port for every client*/
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(0);
    cli_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* Bind the socket to a specific port */
    if (bind(sockfd, (const struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0) {
        printf("\nBind failed\n");
        return -1;
    }

    /* Set IP and Port */
    struct sockaddr_in tmp_addr;
    socklen_t len = sizeof(tmp_addr);
    getsockname(sockfd, (struct sockaddr *) &tmp_addr, &len);
    int client_Port = ntohs(tmp_addr.sin_port);
    *port = client_Port;
    
    return sockfd;
}

// client send
int send_message(char *IP, int Port, char *message)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;

    char port_str[6];
    sprintf(port_str, "%d", Port);

    if ((rv = getaddrinfo(IP, port_str, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // loop through all the results and make a socket
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "talker: failed to create socket\n");
        exit(1);
    }

    if ((numbytes = sendto(sockfd, message, strlen(message), 0,
                           p->ai_addr, p->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }

    freeaddrinfo(servinfo);

    printf("talker: sent %d bytes to port %d\n", numbytes, Port);
    // close(sockfd);
    return sockfd;
}

// client receive message
void receive_udp_message(int sockfd, char* message) {
    char buf[MAX_MESSAGE_LENGTH];
    struct sockaddr_storage sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    int numbytes = recvfrom(sockfd, buf, MAX_MESSAGE_LENGTH-1 , 0, (struct sockaddr *)&sender_addr, &addr_len);
    if (numbytes == -1) {
        perror("recvfrom");
        exit(1);
    }
    buf[numbytes] = '\0';
    printf("Received UDP message: %s\n", buf);
    strcpy(message, buf);
}

void receive_udp_message_size(int sockfd, char* message, int size) {
    char buf[MAX_MESSAGE_LENGTH];
    struct sockaddr_storage sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    int numbytes = recvfrom(sockfd, buf, size , 0, (struct sockaddr *)&sender_addr, &addr_len);
    if (numbytes == -1) {
        perror("recvfrom");
        exit(1);
    }
    buf[numbytes] = '\0';
    printf("Received %d UDP message\n", numbytes);
    strcpy(message, buf);
}

void pick_n_idx(int size, int n, int* result) {

    // Pick n random indices from the array
    for (int i = 0; i < n; i++) {
        int index = rand() % size;

        // Check that the index hasn't already been selected
        for (int j = 0; j < i; j++) {
            if (result[j] == index) {
                index = rand() % size;
                j = -1;
            }
        }

        // Assign the selected element to the result array
        result[i] = index;
    }
    printf("random result: ");
    for (int z = 0; z<n; z++){
        printf("%d ", result[z]);
    }
    printf("\n");
}

int argmax(int a1, int a2){
    if (a1 > a2) {
        return a1;
    }
    return a2;
}

// // Encode an array of log structs to a character array
// char* encode(Person* persons, int size) {
//     // Allocate memory for the encoded string
//     char* encoded = malloc(1);

//     // Initialize the encoded string to the empty string
//     encoded[0] = '\0';

//     // Encode each Person struct in the array
//     for (int i = 0; i < size; i++) {
//         // Encode the name and age fields
//         char buffer[100];
//         sprintf(buffer, "%s;%d", persons[i].name, persons[i].age);

//         // Concatenate the encoded Person to the encoded string
//         encoded = realloc(encoded, strlen(encoded) + strlen(buffer) + 2);
//         strcat(encoded, buffer);
//         strcat(encoded, ",");
//     }

//     // Remove the trailing comma from the encoded string
//     encoded[strlen(encoded) - 1] = '\0';

//     return encoded;
// }

// // Decode a character array to an array of integers
// int* decode(char* encoded, int size) {
//     // Allocate memory for the decoded array
//     int* decoded = malloc(sizeof(log_entry) * size);

//     // Copy the integers from the encoded string to the decoded array
//     memcpy(decoded, encoded, sizeof(log_entry) * size);

//     return decoded;
// }
