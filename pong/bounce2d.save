#include	<stdio.h>
#include	<curses.h>
#include	<signal.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	"bounce.h"
#include	"alarmlib.h"

/*
 *	bounce2d 1.0	
 *
 *	bounce a character (default is *) around the screen
 *	defined by some parameters
 *
 *	user input:
 *		 	s slow down x component, S: slow y component
 *		 	f speed up x component,  F: speed y component
 *			Q quit
 *
 *	blocks on read, but timer tick sets SIGALRM which are caught
 *	by ball_move
 */

struct ppball the_ball ;

void set_up();
void wrap_up();
int  bounce_or_lose(struct ppball *);

/** the main loop **/

int main()
{
	int	c;
	void	set_up();

	set_up();

	while ( ( c = getch()) != 'Q' ){
		if ( c == 'f' )	     the_ball.x_delay--;
		else if ( c == 's' ) the_ball.x_delay++;
		else if ( c == 'F' ) the_ball.y_delay--;
		else if ( c == 'S' ) the_ball.y_delay++;
	}
	wrap_up();
	return 0;
}

/*	init ppball struct, signal handler, curses	*/

void set_up()
{
	void	ball_move(int);

	the_ball.y_pos = Y_INIT;
	the_ball.x_pos = X_INIT;
	the_ball.y_count = the_ball.y_delay = Y_DELAY ;
	the_ball.x_count = the_ball.x_delay = X_DELAY ;
	the_ball.y_dir = 1  ;
	the_ball.x_dir = 1  ;
	the_ball.symbol = DFL_SYMBOL ;

	initscr();		/* turn on curses	*/
	noecho();		/* turn off echo	*/
	cbreak();		/* turn off buffering	*/

	signal(SIGINT, SIG_IGN);	/* ignore SIGINT	*/
	mvaddch(the_ball.y_pos, the_ball.x_pos, the_ball.symbol);
	refresh();
	
	signal( SIGALRM, ball_move );
	set_ticker( 1000 / TICKS_PER_SEC );	/* send millisecs per tick */
}

/* stop ticker and curses */
void wrap_up()
{

	set_ticker( 0 );
	endwin();		/* put back to normal	*/
}

/* SIGARLM handler: decr directional counters, move when they hit 0	*/
/* note: may have too much going on in this handler			*/

void ball_move(int s)
{
	int	y_cur, x_cur, moved;

	signal( SIGALRM , SIG_IGN );		/* dont get caught now 	*/
	y_cur = the_ball.y_pos ;		/* old spot		*/
	x_cur = the_ball.x_pos ;
	moved = 0 ;

	if ( the_ball.y_delay > 0 && --the_ball.y_count == 0 ){
		the_ball.y_pos += the_ball.y_dir ;	/* move	*/
		the_ball.y_count = the_ball.y_delay  ;	/* reset*/
		moved = 1;
	}

	if ( the_ball.x_delay > 0 && --the_ball.x_count == 0 ){
		the_ball.x_pos += the_ball.x_dir ;	/* move	*/
		the_ball.x_count = the_ball.x_delay  ;	/* reset*/
		moved = 1;
	}

	if ( moved ){
		mvaddch(y_cur, x_cur, BLANK);
		mvaddch(the_ball.y_pos, the_ball.x_pos, the_ball.symbol);
		bounce_or_lose( &the_ball );
		move(LINES-1, COLS-1);		/* park cursor	*/
		refresh();
	}
	signal(SIGALRM, ball_move);		/* re-enable handler	*/
}

/* bounce_or_lose: if ball hits walls, change its direction
 *   args: address to ppball
 *   rets: 1 if a bounce happened, 0 if not
 */
int bounce_or_lose(struct ppball *bp)
{
	int	return_val = 0 ;

	if ( bp->y_pos == TOP_ROW )
		bp->y_dir = 1 , return_val = 1 ;
	else if ( bp->y_pos == BOT_ROW )
		bp->y_dir = -1 , return_val = 1;

	if ( bp->x_pos == LEFT_EDGE )
		bp->x_dir = 1 , return_val = 1 ;
	else if ( bp->x_pos == RIGHT_EDGE )
		bp->x_dir = -1 , return_val = 1;

	return return_val;
}

