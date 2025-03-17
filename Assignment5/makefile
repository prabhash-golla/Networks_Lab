CC = gcc
CFLAGS = -Wall -Werror -g

# Try to detect Python command
PYTHON := $(shell which python3 2>/dev/null || which python 2>/dev/null)
ifeq ($(PYTHON),)
$(error No Python interpreter found. Please install Python)
endif


run: QueueServer DummyClient1 DummyClient2 DummyClient3 DummyClient4 WorkerClient
	./QueueServer tasks.txt

QueueServer: QueueServer.c
	$(CC) $(CFLAGS) -o QueueServer QueueServer.c

DummyClient1: WorkerClient.c
	$(CC) $(CFLAGS) -DRECIVE -o DummyClient1 WorkerClient.c

DummyClient2: WorkerClient.c
	$(CC) $(CFLAGS) -DREPEATED -o DummyClient2 WorkerClient.c

DummyClient3: WorkerClient.c
	$(CC) $(CFLAGS) -DNORESPONSE -o DummyClient3 WorkerClient.c

DummyClient4: WorkerClient.c
	$(CC) $(CFLAGS) -DWAITRECIVE -o DummyClient4 WorkerClient.c

WorkerClient: WorkerClient.c
	$(CC) $(CFLAGS) -o WorkerClient WorkerClient.c

clean:
	rm -f QueueServer DummyClient1 DummyClient2 DummyClient3 DummyClient4 WorkerClient tasks.txt

run_server: QueueServer
	./QueueServer tasks.txt

run_dummy1: DummyClient1
	./DummyClient1

run_dummy2: DummyClient2
	./DummyClient2

run_dummy3: DummyClient3
	./DummyClient3
	
run_dummy4: DummyClient3
	./DummyClient4

run_worker: WorkerClient
	./WorkerClient

all: QueueServer DummyClient1 DummyClient2 DummyClient3 DummyClient4 WorkerClient db

db:
	$(PYTHON) generate_tasks.py -n 10 -o tasks.txt
