#
# Makefile for the tlast emulator of the system "last" command
#   Builds the target executable and tests it on the supplied
#   sample files, wtmp-[a:h,big], to verify that the output
#   is identical to /bin/last
#

CC = cc -Wall -Wextra -g

test: tarc

tarc: tarc.o tarutils.o msgutils.o
	$(CC)  tarc.o tarutils.o msgutils.o -o tarc

tarc.o: tarc.c
	$(CC) -c tarc.c

tarutils.o: tarutils.c tarutils.h
	$(CC) -c tarutils.c

msgutils.o: msgutils.c msgutils.h
	$(CC) -c msgutils.c

clean:
	rm -f *.o
