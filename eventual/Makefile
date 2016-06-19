OBJS1 = server.o main.o
OBJS2 = client.o main_c.o

CC = g++
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)

all: serv client

serv: $(OBJS1)
	$(CC) $(LFLAGS) -lpthread $(OBJS1) -o serv

main.o: main.cpp server.h
	$(CC) $(CFLAGS) -lpthread main.cpp

server.o: server.cpp server.h
	$(CC) $(CFLAGS) -lpthread server.cpp

client: $(OBJS2)
	$(CC) $(LFLAGS) -lpthread $(OBJS2) -o client

main_c.o: main_c.cpp client.h
	$(CC) $(CFLAGS) -lpthread main_c.cpp

client.o: client.cpp client.h
	$(CC) $(CFLAGS) -lpthread client.cpp

clean:
	\rm *.o *~ serv client