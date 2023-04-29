#include "tool.h"

int main( int argc, char *argv[] ) {


    pthread_t node_node_thread,download_thread;
    int ID; // server ID to join
    char prompt[] = "post/read/choose/reply\n@> ";
    char message[MAX_MESSAGE_LENGTH];

    // Prompt: which server to join {1,2,3,4}
    printf("Prompt: which server to join {0,1,2,3,4}\n");
    scanf("%d",&ID);
    printf("you join server %d\n",ID);

    char func[10];
    char filename[15];
    char buf[MAX_MESSAGE_LENGTH];
    
    while(1){
        
        printf("%s",prompt);
        scanf("%s", func);
        if(strcmp(func,"post")==0){
            printf("title;content\n@> ");
            memset(buf,0,sizeof(buf));
            memset(message,0,sizeof(message));
            scanf("%s",buf);
            strcat(message, "post;");
            strcat(message, buf);
            int sock = send_message(IP, client_ports[ID], message);
            memset(buf,0,sizeof(buf));
            recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
            printf("%s\n", buf);
        }
        else if (strcmp(func,"read") == 0) {
            memset(buf,0,sizeof(buf));
            memset(message,0,sizeof(message));
            strcat(message,"read;");
            int sock = send_message(IP, client_ports[ID], message);
            memset(buf,0,sizeof(buf));
            recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
            printf("%s\n", buf);
        }
        else if (strcmp(func,"choose") == 0) {
            printf("title\n@> ");
            memset(buf,0,sizeof(buf));
            memset(message,0,sizeof(message));
            scanf("%s",buf);
            strcat(message, "choose;");
            strcat(message, buf);
            int sock = send_message(IP, client_ports[ID], message);
            memset(buf,0,sizeof(buf));
            recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
            printf("%s\n", buf);
        }
        else if (strcmp(func,"reply") == 0) {
            printf("<title of article to reply>;content\n@> ");
            memset(buf,0,sizeof(buf));
            memset(message,0,sizeof(message));
            scanf("%s",buf);
            strcat(message, "reply;");
            strcat(message, buf);
            int sock = send_message(IP, client_ports[ID], message);
            memset(buf,0,sizeof(buf));
            recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
            printf("%s\n", buf);
        }

    }
    pthread_join(download_thread,NULL);
}
