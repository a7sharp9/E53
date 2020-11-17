/*
 * drawable.c
 *
 *  Created on: Mar 11, 2018
 *      Author: Yuri
 *
 *  handles curses-based drawing of an object that can have a length along
 *  the y-axis, or printing of a line of characters on a string, while
 *  excluding race conditions (simultaneous updates of the screen)
 */

#include "drawable.h"

#include <curses.h>
#include <pthread.h>

static pthread_cond_t lock;
static pthread_mutex_t mutex;

#define BLANK ' '

/**
 * init_drawable - initializes the mutex lock
 */
void init_drawable () {
	pthread_mutex_init (&mutex, NULL);
	pthread_cond_init (&lock, NULL);
}

/**
 * mvaddstr_exclusive - wrapper around mvaddstr that performs the actual
 * printing between acquiring and releasing a mutex - to prevent other
 * changes to the screen happening at the same time
 * args:
 * y, x, msg - passed through to mvaddstr
 * return:
 * the return value from mvaddstr
 */
int mvaddstr_exclusive (int y, int x, char *msg) {
	pthread_cond_wait (&lock, &mutex); // wait for and acquire the lock
	int ret = mvaddstr (y, x, msg);
	move (0, 0); // park
	pthread_mutex_unlock (&mutex); // release the lock
	refresh ();
	return (ret);
}


/**
 * draw_moveable - consecutively draws the characters of an object from
 * top to bottom, refreshes the screen and parks the cursor.
 * to exclude race conditions, locks on a mutex resource, so that subsequent
 * calls to it do not attempt to update the screen while the execution
 * is still inside the drawing code
 * args:
 * moveable - the object to draw
 * erase - draw the current position with the object's character if 0,
 * or with blanks otherwise
 */
void draw_moveable (struct drawable const *obj, int erase) {
	pthread_cond_wait (&lock, &mutex); // wait for and acquire the lock
	int length_idx;

	// cycle through the constituent characters
	for (length_idx = 0; length_idx < obj->ylength; length_idx++) {
		// move the cursor to the position of the character and draw
			mvaddch (obj->y + length_idx, obj->x,
					erase ? BLANK : obj->symbol);
	}
	move (0, 0); // park
	pthread_mutex_unlock (&mutex); // release the lock

	refresh ();
}


