test: server.out client.out
	./server.out &
	./client.out
server.out: wordserver.c
	gcc -o server.out wordserver.c
client.out: wordclient.c
	gcc -o client.out wordclient.c
clean:
	- rm -f *.out *\(copy\).txt