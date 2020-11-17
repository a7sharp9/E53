#include 	"drawable.h"
#include    "ball.h"
#include    "paddle.h"

#include    <curses.h>
#include    <signal.h>
#include    <string.h>
#include    <unistd.h>
#include    <stdlib.h>
#include	<stdio.h>
#include	<errno.h>
#include	<time.h>
#include	<sys/ioctl.h>

static struct game {
	int in_play;	// to prevent additional serves (if true)
					// and ball event handling and paddle moves (if false)
	int serves_left; // not counting the ball in play
	struct boundaries boundaries;
} pong_game;

#define BOUNDARY_OFFSET 3	// off screen margins
#define NUM_INITIAL_SERVES 3 // start with this number of balls
#define COURT_HORIZONTAL_SYMBOL '-'
#define COURT_VERTICAL_SYMBOL '|'

/**
 * print_serves_left - generates a string indicating the number of
 * serves left (not including current ball in play, if any) and
 * displays it on the left side of the screen above the top
 * boundary of the court
 * args:
 * boundaries - screen coordinates of the court's margins
 * serves_left - the number to display
 */
static void print_serves_left (struct boundaries *boundaries, int serves_left) {
	int half_width =
			(boundaries->right_edge - boundaries->left_edge) / 2;
	char msg [half_width];

	if (half_width < 2) // can't display anything
		return;

	int str_pos = snprintf (msg, half_width - 2, "%s", "Serves left: ");
	sprintf (msg + str_pos, "%d", serves_left);
	mvaddstr (boundaries->top_row > 0 ?
			boundaries->top_row -1 : 0, boundaries->left_edge, msg);
	refresh ();
}

/**
 * draw_field - puts a visual representation of the court on screen
 * args:
 * boundaries - screen coordinates of the court's margins
 */
static void draw_field (struct boundaries *boundaries) {
	int field_length = boundaries->right_edge -boundaries->left_edge;

	char top_bottom [field_length + 1]; // string for horizontal margins
	memset (top_bottom, COURT_HORIZONTAL_SYMBOL, field_length);
	top_bottom [field_length] = 0;

	mvaddstr (boundaries->top_row, boundaries->left_edge,
							top_bottom); // draw top margin
	mvaddstr (boundaries->bottom_row, boundaries->left_edge,
							top_bottom); // draw bottom margin

	int left_idx;
	for (left_idx = boundaries->top_row;
		 left_idx <  boundaries->bottom_row;
		 left_idx ++) // draw left margin top to bottom
	{
		mvaddch (left_idx, boundaries->left_edge, COURT_VERTICAL_SYMBOL);
	}

	// add game instructions string below bottom margin
	mvaddstr (boundaries->bottom_row + 1, boundaries->left_edge,
			 "'s' - serve, 'k' - paddle up, 'm' - paddle down, 'Q' - quit");

	refresh ();
}

/**
 * set_boundaries - initializes coordinates of the court's margins depending
 * on the screen size and fills the incoming struct
 * args:
 * boundaries - pointer to the holder of margin info
 */
static void set_boundaries (struct boundaries *boundaries) {
	struct winsize size;

	if (ioctl (STDIN_FILENO, TIOCGWINSZ, &size)) { // get screen size
		perror ("can't create the court");
		exit (1);
	}

	// try to create a court that's offset by BOUNDARY_OFFSET from
	// top and bottom; if that won't fit, use screen margins
	int vertical_boundary = (size.ws_row > 2 * BOUNDARY_OFFSET + 1) ?
									BOUNDARY_OFFSET : 0;
	int horizontal_boundary = (size.ws_col > 2 * BOUNDARY_OFFSET + 1) ?
									BOUNDARY_OFFSET : 0;

	*boundaries = (struct boundaries) { // initialize the struct
		vertical_boundary, size.ws_row - vertical_boundary,
		horizontal_boundary, size.ws_col - horizontal_boundary
	};
}

/**
 * print_end_msg - outputs the "game over" message, including the
 * cumulative time achieved
 */
static void print_end_msg () {
	int mid_y = // middle row
		(pong_game.boundaries.top_row + pong_game.boundaries.bottom_row) /2;
	int court_len = pong_game.boundaries.right_edge -
		  	  pong_game.boundaries.left_edge;
	char msg [court_len  >= MAXTIMELEN ? court_len + 1 : MAXTIMELEN];
	// fit at least the time

	snprintf (msg, court_len - MAXTIMELEN, "%s", "Game over. Your time is ");
	format_elapsed (msg + strlen (msg)); // add the cumulative time in MM:SS

	// print the ending message
	mvaddstr (mid_y, pong_game.boundaries.left_edge + 1, msg);
	refresh ();
}

/**
 * prepare_serve - sets the court boundaries (or resets in case the screen size
 * changed), redraws the court, updates the number of serves left, and then
 * depending on that either prints "game over" message or initializes
 * the ball and paddle positions
 */
static void prepare_serve () {
	clear ();
	set_boundaries (&pong_game.boundaries); // will ask ioctl for size
	draw_field (&pong_game.boundaries);		// redraw court
	print_serves_left (&pong_game.boundaries, pong_game.serves_left);
	print_elapsed_seconds (); // total seconds played
	if (pong_game.serves_left <= 0)
		print_end_msg (); // game over
	else { // reinit ball and paddle
		paddle_init (&pong_game.boundaries);
		ball_init (&pong_game.boundaries);
	}
}

/**
 * court_resize - handler for the window size change signal
 * if not in play, queries the new size and redraws the court
 * Note: no need to obtain mutex on screen redraws, since nothing
 * else is redrawing it when the ball is not in play
 */
static void court_resize () {
	if (!pong_game.in_play) {
		signal (SIGWINCH, SIG_IGN); // to prevent resize during redraw
		prepare_serve (); // ask for new size and draw
		signal (SIGWINCH, court_resize); // reset handler
	}
}

/**
 * serve - starts the ticker, decreases the remaining serve count,
 * sets alarm handler to the ball moving function
 */
static void serve () {
	signal (SIGALRM, ball_move); // handler for the alarm from the ticker
	print_serves_left (&pong_game.boundaries, --pong_game.serves_left);
						// decrease serve count
	pong_game.in_play = 1; // no new serves
	ticker_set (); // start generating ALRM signals
}

/**
 * set_up - initial set-up of curses, seeding the pseudorandom
 * generator, initializing mutex for screen updates, displaying the game
 */
static void set_up () {
	initscr ();
	noecho ();
	crmode ();
	init_drawable (); // initialize mutex

	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
	signal (SIGWINCH, court_resize); // redraw the court when not in play

	srandom (time (NULL)); // seed

	pong_game = (struct game) {0, NUM_INITIAL_SERVES, {0, 0, 0, 0}}; //init

	prepare_serve (); // draw the screen and messages
}

/**
 * wrap_up - stops the ticker and folds up the game screen
 */
static void wrap_up () {
	ticker_quit ();
	endwin ();
}

/**
 * bounce_or_lose - polls the ball position, and if it indicates that
 * the ball went off-field, pauses the game and prepares for next serve,
 * if any are left
 */
void bounce_or_lose () {
	if (ball_bounce () < 0) {
		ticker_quit (); // stop generating ALRM signals
		pong_game.in_play = 0; // enable user input for the next serve
		prepare_serve (); // reinitialize the court and positions of
							// the ball and the paddle
	}
}

/**
 * main - blocks for user input, handling paddle moves and serve requests
 */
int main () {
	set_up (); // initialize and display the field, set haldlers

	int c;
	while ((c = getchar ()) != 'Q') { // block on input until quit
		switch (c) {
			case 'm':	// paddle down
			case 'k':	// paddle up
				if (pong_game.in_play) { // in theory, could allow the paddle
				// to move always, but then might race with the court redraw
				// upon screen resize events
					c == 'm' ? paddle_down () : paddle_up ();
					bounce_or_lose (); // check for contact with ball
				}
				break;

			case 's': // serve
				if (!pong_game.in_play && pong_game.serves_left > 0)
					serve (); // start the game round
				break;
		}
	}

	wrap_up (); // return to previous screen contents

	return (0);
}

