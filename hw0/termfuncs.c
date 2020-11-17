#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

/*
 * termfuncs.c  -- version 1.0
 *
 *  purpose: provide access to terminal input and terminal information
 * contains: rawgetc(FILE *f)        -- read one char from device at *f
 *           get_term_size(int a[2]) -- put term rows, cols in array a
 */

/*
 * get_term_size -- returns #rows and #cols in an array
 *     args: array to hold two ints
 *     rets: 0 if ok, -1 if error
 *     note: sets errno via ioctl, 
 *           caller is responsible for error reporting
 */
int get_term_size( int rows_cols[2] )
{
	struct winsize w;
	int rv;

	rv = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	if ( rv == 0 ){
		rows_cols[0] = w.ws_row;
		rows_cols[1] = w.ws_col;
	}
	return rv;  
}

/*
 * rawgetc -- reads and returns one char from a terminal device
 *   args: FILE * that refers to the device
 *   rets: a single char or EOF for  EOF -2 if error
 *   note: turns off echo and turns off need for Enter to send char
 */
int rawgetc(FILE *f)
{
	struct termios orig, modified;
	char	c;
	int	rv;
	int	fd = fileno(f);

	fflush (stdout);
	if ( tcgetattr(fd, &orig) == -1 )	/* get settings	*/
	{
		perror("Reading settings on input");
		exit(1);
		// return -2;
	}
	modified = orig;			/* make working copy	*/
	modified.c_lflag   &= ~ICANON;		/* no buffering		*/
	modified.c_lflag   &= ~ECHO;		/* no echoing please	*/
	modified.c_cc[VMIN] = 1;		/* read chars 1-by-1	*/
	tcsetattr(fd, TCSANOW, &modified);	

	rv = read( fd, &c, 1);			/* read a char		*/

	tcsetattr(fd, TCSANOW, &orig) ;

	if ( rv == 1 )
		return c;			/* return char if avail	*/
	return EOF;				/* else say EOF		*/
}
	
