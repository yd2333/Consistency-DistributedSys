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

## Test cases
All test cases are for 3 servers.
### Primary backup
1. run the following 3 commands in 3 different tabs with ID configuration. Run specifically in the following order\
1st
```
./server 0
```
2nd
```
./server 1
```
3rd
```
./server 2
```
2. Protocol configuration
### server 0, 1, 2
```
strategy? 1 primary_backup/2 quorum/3 local_write
@> 1
```
3. System will prompt your to configure server identity, 1 for primary and 0 for replica
### server 0, 1
```
primary? (1/0)
@> 0
```
### server 2
```
primary? (1/0)
@> 1
```
4. Run client, then choose a random server to join. The following is an example of joining server 0: \
enter ```0``` to join server 0
```
./client
Prompt: which server to join {0,1,2,3,4}
0
```
Next, enter post to post to the system
```
you join server 0
post/read/choose/reply
@> post
title;content
@> ArticleTitle;article
```
Then, you will expect to see result on the screen
```
@> ArticleTitle;article
talker: sent 26 bytes to port 51599
ack
```
which means your post is on the server. Continue to do the same post operation twice, then you can read the list that is on the server.
```
post/read/choose/reply
@> read
talker: sent 5 bytes to port 51599
-------------------
0 - ArticleTitle
1 - 111
2 - 444

-------------------
```
fetch the content of the title like following
```
post/read/choose/reply
@> choose 
title
@> ArticleTitle
talker: sent 20 bytes to port 51599
-------------------
article

-------------------
```
try replying
```
post/read/choose/reply
@> reply
<title of article to reply>;title;content
@> ArticleTitle;Reply0;This is the reply
talker: sent 44 bytes to port 51599
ack
```
Then you can see reply appear on the server
```
post/read/choose/reply
@> read
talker: sent 5 bytes to port 51599
-------------------
0 - ArticleTitle
----3 - Reply0
1 - 111
2 - 444

-------------------
```
terminate ./client. When log back to any of other servers, you will see the exact same result
```
@> ^C
wan00807@csel-kh1250-05:/home/wan00807/5105/Project2_Consistency/Project2_Consistency $ ./client
Prompt: which server to join {0,1,2,3,4}
2
you join server 2
post/read/choose/reply
@> read
talker: sent 5 bytes to port 45693
-------------------
0 - ArticleTitle
----3 - Reply0
1 - 111
2 - 444

-------------------
```
### Quorum Consistency
Running quorum is similar except: \
1. choose 2 in the strategy selection \
2. When configuring primary, you see a the system asking for number of write quorum and read quorum
```
 ./server 2
strategy? 1 primary_backup/2 quorum/3 local_write
@> 2
primary? (1/0)
@> 1
...
nw nr?
@> 3 1
```
### local write
Here is a test case for posting with local write protocol
```
 ./client
Prompt: which server to join {0,1,2,3,4}
0
you join server 0
post/read/choose/reply
@> post
title;content
@> Title01;Content.
talker: sent 22 bytes to port 51599
update success (broadcast on backend...)
```
As you terminate the client, then log back to other server, say server 1, you will see the same result
```
./client
Prompt: which server to join {0,1,2,3,4}
1
you join server 1
post/read/choose/reply
@> read
talker: sent 5 bytes to port 30883
-------------------
0 - Title01

-------------------
```

## Design document
In the file server, we implemented 3 different consistency strategies: Sequential consistency with primary-backup protocal, quorum consistency and Read your write consistency. \
### Primary-backup
This function wrap the functionality of writing operations with primary backup protocal 
```
void primary_backup_write(char* request, char* answer);
```
### Quorom
```int find_version_wiz_title(char* title)``` is for broadcasting for proposal of version:content pair \
```void read_with_vote(char* var, char* answer)``` is for broadcasting for read proposal of latest content \
```int check_avalability(char* message)``` is for check the posibility of updating content \
```int ask_quorum_vote(char* request) ``` is for asking the quorum to vote for a write request from primary (proposed by client) \

### Local-write
handle incoming request with local write \
request: request from client \
answer: store the answer to client 
```
void local_write(char* request, char* answer)
```
### Other functionality
```void* broadcast_threading(void* input)```: create a thread for broadcasting \
```int update_write(char* message)``` is for updating the write request to the local copy of logs \
```void choose(char* message, char* answer)```: find content with title \
```void* replica_listen_request()```: thread handling incoming request from primary, used by replica \
```void* primary_listen_request()```: thread handling incoming request from replica, used by primay \
```int broadcast_primary()```: broadcasting primary identity to all of the rest servers, including exiisting primary 


## Test result with 3 servers (in seconds)
Primary post: 0.000107 seconds \
Primary read: 0.000019 seconds \
Quorum post: 0.000053 seconds \
Quorum read:  0.000054 seconds \
Primary post: 0.002976 seconds \
Primary read: 0.000013 seconds 