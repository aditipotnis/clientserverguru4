server: server.o database.o
	gcc -o server server.o database.o

server.o: server.c database.h
	gcc -c server.c

client: client.o database.o
	gcc -o client client.o database.o

client.o: client.c database.h
	gcc -c client.c


