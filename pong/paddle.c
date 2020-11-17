/*
 * paddle.c
 *
 *  Created on: Mar 11, 2018
 *      Author: Yuri
 */

#include "paddle.h"
#include <signal.h>
#include <stddef.h>
#include <curses.h>
#include "drawable.h"

static struct paddle {
	struct drawable position; // the current position of the paddle's top
								// and its length in y-direction
	struct boundaries boundaries; // boundaries of the game court
	int ydirection; // whether moving up (-1) or down (1)
} paddle = {{-1, -1, 0, 0}, {0, 0, 0, 0}, 1};

#define PADDLE_CHAR   '#'
#define PADDLE_LENGTH_RELATIVE_HEIGHT 3

/**
 * move_paddle - try to move the paddle in the specified direction, redrawing
 * it in the new position if the move is possible
 * Note: uses draw_moveable, which acquires and releases a mutex to prevent
 * concurrent screen updates
 */
static void move_paddle () {
	struct drawable *paddle_position = &paddle.position;
	struct boundaries *boundaries = &paddle.boundaries;

	// check if we're already bumping against the edge in the
	// direction we want to move
	int can_move = (paddle.ydirection > 0) ?
			paddle_position->y + paddle_position->ylength <
				boundaries->bottom_row: // going down, check bottom
				paddle_position->y > boundaries->top_row + 1;
													// going up, check top

	if (can_move) { // there's enough space
		draw_moveable (paddle_position, 1); // erase previous paddle
		paddle_position->y += paddle.ydirection; // move
		draw_moveable (paddle_position, 0); // draw paddle in the new position
	}
}

/**
 * paddle_up - a wrapper around move_paddle that attempts the move in the
 * negative y-direction
 */
void paddle_up () {
	paddle.ydirection = -1;
	move_paddle ();
}

/**
 * paddle_down - a wrapper around move_paddle that attempts the move in the
 * positive y-direction
 */

void paddle_down () {
	paddle.ydirection = 1;
	move_paddle ();
}

/**
 * paddle_init - given the dimensions of the game court, initialize
 * the paddle length, place it at the middle of the court on the right
 * edge and draw it
 */
void paddle_init (struct boundaries const *boundaries) {
	if (paddle.position.x >= 0) // paddle have been drawn before
		draw_moveable (&paddle.position, 1); // erase current position

	int paddle_length = (boundaries->bottom_row - boundaries->top_row) /
					PADDLE_LENGTH_RELATIVE_HEIGHT; // define length

	paddle = (struct paddle) { // init and place in the middle
					{boundaries->right_edge,
			(boundaries->bottom_row + boundaries->top_row - paddle_length) / 2,
					PADDLE_CHAR, paddle_length},
					*boundaries, 1};

	draw_moveable (&paddle.position, 0); // draw in new position
}

/**
 * paddle_contact - for given ball coordinates, see whether it is making
 * contact with the paddle
 * args:
 * y, x - coordinates of the ball
 * return:
 * 0 if there is no paddle at these coordinates, non-0 otherwise
 */
int paddle_contact (int y, int x) {
	int ret = (x == paddle.position.x && // in the same column
			y >= paddle.position.y && // below the row of the paddle top
			y <= paddle.position.y + paddle.position.ylength); // above bottom

	return (ret);
}


