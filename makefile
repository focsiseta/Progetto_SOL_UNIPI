OBJS	= ./bin/log.o ./bin/parser.o ./bin/queue.o ./bin/server.o ./bin/utils.o ./bin/server_utils.o ./bin/memory.o ./bin/prot.o ./bin/clientutils.o
SOURCE	= ./src/log.c ./src/parser.c ./src/queue.c ./src/server.c ./src/utils.c ./src/server_utils.c ./src/memory.c ./src/prot.c ./src/clientutils.c
HEADER	= ./headers/log.h ./headers/parser.h ./headers/queue.h ./headers/clientutils.h ./headers/server.h ./headers/utils.h ./headers/server_utils.h ./headers/memory.h ./headers/prot.h
OUT	= server
CC	 = gcc
FLAGS	 = -g -c -Wall -lpthread
LFLAGS	 = -lpthread -g


all: buildserver compile_client



buildserver: $(OBJS)
	mv *.o  ./bin/ && $(CC) -g -lpthread $(OBJS) -g -o $(OUT) $(LFLAGS)

./bin/server_utils.o: ./src/server_utils.c
	$(CC) $(FLAGS) ./src/server_utils.c

./bin/log.o: ./src/log.c
	$(CC) $(FLAGS) ./src/log.c

./bin/parser.o: ./src/parser.c
	$(CC) $(FLAGS) ./src/parser.c

./bin/queue.o: ./src/queue.c
	$(CC) $(FLAGS) ./src/queue.c

./bin/server.o: ./src/server.c
	$(CC) $(FLAGS) ./src/server.c

./bin/utils.o: ./src/utils.c
	$(CC) $(FLAGS) ./src/utils.c

./bin/memory.o: ./src/memory.c
	$(CC) $(FLAGS) ./src/memory.c

./bin/prot.o: ./src/prot.c
	$(CC) $(FLAGS) ./src/prot.c

./bin/clientutils.o: ./src/clientutils.c
	$(CC) $(FLAGS) ./src/clientutils.c






clean:
	rm -f $(OBJS) $(OUT) && rm *log*

start:
	./server config.txt

compile_client:
	gcc ./bin/clientutils.o ./bin/prot.o ./bin/utils.o ./src/client.c -lpthread -g -o client

test1:
	./test1.sh

test2:
	./test2.sh

.PHONY: start clean compile_client
