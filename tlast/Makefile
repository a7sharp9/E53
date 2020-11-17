#
# Makefile for the tlast extension to more02.c
#   This is a minimal Makefile.  If you know more about make
#   feel free to add fancier features.
#

CC = cc -Wall -Wextra -g

tlast: tlast.o wtmputils.o
	$(CC)  tlast.o wtmputils.o -o tlast

tlast.o: tlast.c
	$(CC) -c tlast.c

wtmputils.o: wtmputils.c
	$(CC) -c wtmputils.c

clean:
	rm -f *.o
