CC = gcc

CMDS = c s 

all : $(CMDS)

c : clnt.c
	gcc -o c clnt.c -lpthread

s: server.c
	gcc -o s server.c -lpthread
