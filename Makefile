unibox: main.c unibox.c
	gcc -O0 -g -Wall -lcrypto -lssl -o bin/unibox main.c unibox.c
