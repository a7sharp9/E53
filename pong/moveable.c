/*
 * moveable.c
 *
 *  Created on: Mar 11, 2018
 *      Author: Yuri
 */

#include "moveable.h"
#include <curses.h>
#include <pthread.h>

static pthread_cond_t lock;
static pthread_mutex_t mutex;

#define BLANK       ' '

void init_moveable () {
	pthread_mutex_init (&mutex, NULL);
	pthread_cond_init (&lock, NULL);

}

void draw_moveable (struct moveable *moveable, int erase) {
	pthread_cond_wait (&lock, &mutex);
	int length_idx;
	for (length_idx = 0; length_idx < moveable->ylength; length_idx++) {
			mvaddch (moveable->y + length_idx, moveable->x,
					erase ? BLANK : moveable->symbol);
	}
	pthread_mutex_unlock (&mutex);
	move (0, 0);
	refresh ();
}


