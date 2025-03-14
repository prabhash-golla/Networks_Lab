CC = gcc
CFLAGS = -Wall -Werror -g
PYTHON = python3

all: QueueServer DummyClient WorkerClient

QueueServer: QueueServer.c
	$(CC) $(CFLAGS) -o QueueServer QueueServer.c

DummyClient: DummyClient.c
	$(CC) $(CFLAGS) -o DummyClient DummyClient.c

WorkerClient: WorkerClient.c
	$(CC) $(CFLAGS) -o WorkerClient WorkerClient.c

clean:
	rm -f QueueServer DummyClient WorkerClient
	rm -f tasks.txt

run_server: QueueServer
	./QueueServer

run_dummy: DummyClient
	./DummyClient

run_worker: WorkerClient
	./WorkerClient

gen_tasks:
	python3 generate_tasks.py -n 10 -o tasks.txt