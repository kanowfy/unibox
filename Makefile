unibox: main.c unibox.c attachment.c
	gcc -g -Wall -lcrypto -lssl -o bin/unibox main.c attachment.c unibox.c
