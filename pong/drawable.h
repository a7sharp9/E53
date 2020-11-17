/*
 * moveable.h
 *
 *  Created on: Mar 11, 2018
 *      Author: Yuri
 *
 */

#ifndef MOVEABLE_H
#define MOVEABLE_H

struct drawable {
	int x; // current position of the object along x-axis
	int y; // current position of the top of the object along y-axis
	char symbol; // the symbol to draw on the screen
	int ylength; // the length of the moveable object along y-axis
};

void draw_moveable (struct drawable const *object, int erase);
int mvaddstr_exclusive (int y, int x, char *msg);
void init_drawable ();

#endif /* MOVEABLE_H */
