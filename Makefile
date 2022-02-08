CC = gcc

all: myclient myserver

myclient: client.o
	gcc client.o -o ./bin/myclient
client.o: ./src/client.c
	gcc -c ./src/client.c
myserver: server.o
	gcc server.o -o ./bin/myserver
server.o: ./src/server.c
	gcc -c ./src/server.c
clean:
	rm *.o ./bin/myclient ./bin/myserver
