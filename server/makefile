CC=g++
FLAGS= -Wall -fmax-errors=3 -g 
LIBS= -lWs2_32 -lwsock32 -lboost_filesystem-mt

all: server

server: src/server.cpp
	$(CC) $(FLAGS) src/server.cpp $(LIBS) -o server.exe

