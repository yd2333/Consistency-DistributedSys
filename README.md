# Project 2: Consistency

## How to compile and run

1. decide number of server and config ```server_num``` in ```server.c``` \
```
#define server_num 3
```

2. run replica first, run server at the end \

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
./server primary local_write 4000
./server 1 local_write 4001
./client
```

## Design document
In the file server, we implemented 3 different consistency strategies: Sequential consistency with primary-backup protocal, quorum consistency and Read your write consistency. \
Primary-backup
```
void primary(char* message) 
int primary_backup()

```
Quorom
```
void quorum(char *type, char *title, char *content, int N, int NR, int NW)
void broadcast_to_quorum(int count)
```
Local-write
```
void local_write(char* type,char* title,char* content)
```