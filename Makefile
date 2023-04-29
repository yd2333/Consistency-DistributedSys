server: server.c
	gcc -pthread server.c -o server

client: client.c
	gcc client.c -o client

.PHONY: all run_server run_client clean

all: server client

run_server: server
	./server

run_client: client
	./client

clean:
	rm -f server client