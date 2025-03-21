test: server.out client.out

server.out: mysmtp_server.c
	gcc -o mysmtp_server.out mysmtp_server.c

client.out: mysmtp_client.c
	gcc -o mysmtp_client.out mysmtp_client.c

clean:
	- rm -f *.out
