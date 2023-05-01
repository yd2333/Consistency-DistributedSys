# Project 2: Consistency

## How to compile and run

1. decide number of server and config ```server_num``` in ```server.c``` \
```
#define server_num 3
```

2. run all the replica first, run server at the end \

3. title name should not contain space \

make
```
make all
make server
make client
```

run server \
ID: 1,2,3,4,5 \
```
./server <ID>
```

run client and follow the prompt
```
./client
```

example:
```
./server 0
./server 1 
./client
```

## Design document
In the file server, we implemented 3 different consistency strategies: Sequential consistency with primary-backup protocal, quorum consistency and Read your write consistency. \
### Primary-backup
This function wrap the functionality of writing operations with primary backup protocal 
```
void primary_backup_write(char* request, char* answer);
```
### Quorom
```int find_version_wiz_title(char* title)``` is for broadcasting for proposal of version:content pair
```void read_with_vote(char* var, char* answer)``` is for broadcasting for read proposal of latest content
```int check_avalability(char* message)``` is for check the posibility of updating content
```int ask_quorum_vote(char* request) ``` is for asking the quorum to vote for a write request from primary (proposed by client) \

### Local-write
handle incoming request with local write \
request: request from client \
answer: store the answer to client 
```
void local_write(char* request, char* answer)
```
```void* broadcast_threading(void* input)```: create a thread for broadcasting
### Other functionality
```int update_write(char* message)``` is for updating the write request to the local copy of logs
```void choose(char* message, char* answer)```: find content with title 
```void* replica_listen_request()```: thread handling incoming request from primary, used by replica
```void* primary_listen_request()```: thread handling incoming request from replica, used by primay
```int broadcast_primary()```: broadcasting primary identity to all of the rest servers, including exiisting primary


## Test result with 3 servers (in seconds)
Primary post: 0.000107 seconds
Primary read: 0.000019 seconds
Quorum post: 0.000053 seconds
Quorum read:  0.000054 seconds
Primary post: 0.002976 seconds
Primary read: 0.000013 seconds