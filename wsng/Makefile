#
#
# makefile for webserver
#

CC = gcc -Wall

wsng: wsng.o socklib.o process.o read.o 
	$(CC) -o wsng wsng.o socklib.o process.o read.o 

wsng.o: wsng.c
	$(CC) -c wsng.c -o wsng.o

read.o: read.c read.h
	$(CC) -c read.c -o read.o

process.o: process.c process.h
	$(CC) -c process.c -o process.o

socklib.o: socklib.c socklib.h
	$(CC) -c socklib.c -o socklib.o

clean:
	rm -f *.o
