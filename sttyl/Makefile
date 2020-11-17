#
# Makefile for the sttyl reduced functionality emulator of /bin/stty
#

CC = cc -Wall -Wextra -g

sttyl: sttyl.o ttyutils.o
	$(CC)  sttyl.o ttyutils.o -o sttyl

sttyl.o: sttyl.c ttyutils.h
	$(CC) -c sttyl.c

ttyutils.o: ttyutils.c ttyutils.h
	$(CC) -c ttyutils.c

clean:
	rm -f *.o
