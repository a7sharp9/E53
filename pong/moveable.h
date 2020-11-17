/*
 * moveable.h
 *
 *  Created on: Mar 11, 2018
 *      Author: Yuri
 */

#ifndef MOVEABLE_H
#define MOVEABLE_H

struct moveable {
	int x;
	int y;
	char symbol;
	int ydirection;
	int ylength;
};

void draw_moveable (struct moveable *object, int erase);
void init_moveable ();

#endif /* MOVEABLE_H */
