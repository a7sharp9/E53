/*
 * boundaries.h
 *
 *  Created on: Mar 12, 2018
 *      Author: Yuri
 */

#ifndef BOUNDARIES_H
#define BOUNDARIES_H

struct boundaries { // the coordinates of the boundaries of the game court
	int top_row;  // y-coordinate of the top boundary
	int bottom_row; // y-coordinate of the bottom boundary
	int left_edge; // x-coordinate of the left boundary
	int right_edge; // x-coordinate of the right boundary
};


#endif /* BOUNDARIES_H */
