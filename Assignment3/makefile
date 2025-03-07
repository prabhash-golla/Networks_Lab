test: server.out client.out
	./server.out
server.out: doencfileserver.c
	gcc -o server.out doencfileserver.c
client.out: retrieveencfileclient.c
	gcc -o client.out retrieveencfileclient.c
clean:
	- rm -f *.out *.txt.enc *.*.txt *.*.txt.enc