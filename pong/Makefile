#
# Makefile for the sttyl reduced functionality emulator of /bin/stty
#

CC = cc -Wall -Wextra -g

pong: pong.o ball.o paddle.o drawable.o alarmlib.o
	$(CC)  pong.o ball.o paddle.o drawable.o alarmlib.o -lcurses -o pong

pong.o: pong.c ball.h paddle.h drawable.h alarmlib.h
	$(CC) -c pong.c -o pong.o

paddle.o: paddle.c paddle.h drawable.h
	$(CC) -c paddle.c -o paddle.o

ball.o: ball.c ball.h drawable.h
	$(CC) -c ball.c -o ball.o

drawable.o: drawable.c drawable.h
	$(CC) -c drawable.c -o drawable.o

alarmlib.o: alarmlib.c alarmlib.h
	$(CC) -c alarmlib.c

clean:
	rm -f *.o
