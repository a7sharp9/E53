#
# Makefile for the tlast emulator of the system "last" command
#   Builds the target executable and tests it on the supplied
#   sample files, wtmp-[a:h,big], to verify that the output
#   is identical to /bin/last
#

CC = cc -Wall -Wextra -g

smsh: smsh.o smsh5.o builtin.o controlflow.o flexstr.o process.o splitline.o varlib.o
	$(CC)  smsh.o smsh5.o builtin.o controlflow.o flexstr.o process.o splitline.o varlib.o -o smsh

smsh.o: smsh.c smsh.h
	$(CC) -c smsh.c

smsh5.o: smsh5.c
	$(CC) -c smsh5.c

builtin.o: builtin.c builtin.h
	$(CC) -c builtin.c

controlflow.o: controlflow.c controlflow.h
	$(CC) -c controlflow.c

process.o: process.c process.h
	$(CC) -c process.c

splitline.o: splitline.c splitline.h
	$(CC) -c splitline.c

flexstr.o: flexstr.c flexstr.h
	$(CC) -c flexstr.c

varlib.o: varlib.c varlib.h
	$(CC) -c varlib.c

clean:
	rm -f *.o
