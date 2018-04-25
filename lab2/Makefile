cclient : cclient.o connections.o gethostbyname6.o linkedlist.o
	gcc -Wall -pedantic -std=gnu99 cclient.o connections.o gethostbyname6.o linkedlist.o -o cclient

server : server.o connections.o gethostbyname6.o linkedlist.o
	gcc -Wall -pedantic -std=gnu99 server.o connections.o gethostbyname6.o linkedlist.o -o server

server.o : server.h server.c
	gcc -Wall -pedantic -std=gnu99 server.c -c

cclient.o : cclient.h cclient.c
	gcc -Wall -pedantic -std=gnu99 cclient.c -c

connections.o : connections.h connections.c
	gcc -Wall -pedantic -std=gnu99 connections.c -c

gethostbyname6.o : gethostbyname6.h gethostbyname6.c
	gcc -Wall -pedantic -std=gnu99 gethostbyname6.c -c

linkedlist.o : linkedlist.h linkedlist.c
	gcc -Wall -pedantic -std=gnu99 linkedlist.c -c

clean :
	rm *.o
	rm cclient
	rm server
