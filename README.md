# Project 2: Consistency

## How to compile and run
make
```
make all
make server
make client
```

run server \
argv[1] : "primary" if running a primary server; anything else would be replica\
argv[2] : {"local_write", "primary", "quorum"}\
argv[3] : port 4000
```
./server <primary/replica> <strategy> <port>
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