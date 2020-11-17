#
# Makefile for the more03 extension to more02.c
#   This is a minimal Makefile.  If you know more about make
#   feel free to add fancier features.
#

CC = cc -Wall -Wextra -g

more03: more03.o termfuncs.o
	$(CC)  more03.o termfuncs.o -o more03

more03.o: more03.c
	$(CC) -c more03.c

termfuncs.o: termfuncs.c
	$(CC) -c termfuncs.c

clean:
	rm -f *.o
