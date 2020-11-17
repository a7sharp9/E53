/*  more03.c  - version 0.3 of more
 *	read and print as many lines ts the current terminal has, then pause for a few special commands
 *	v03: reads user control cmds from /dev/tty, dynamically polls the terminal size
 */
#include <stdio.h>
#include <stdlib.h>
#include "termfuncs.h"
#define  ERROR		1
#define  SUCCESS	0
#define	DEFAULT_PAGELEN 24
#define	DEFAULT_PAGEWIDTH 80
#define  has_more_data(x)   (!feof(x))
#define	CTL_DEV	"/dev/tty"		/* source of control commands	*/

int  do_more(FILE *);
int  how_much_more(FILE *);
int print_one_line(FILE *);
int pagelen ();
int pagewidth ();

int main( int ac , char *av[] )
{
	FILE	*fp;			/* stream to view with more	*/
	int	result = SUCCESS;	/* return status from main	*/

	if ( ac == 1 )
		result = do_more( stdin );
	else
		while ( result == SUCCESS && --ac )
			if ( (fp = fopen( *++av , "r" )) != NULL ) {
				result = do_more( fp ) ; 
				fclose( fp );
			}
			else
				result = ERROR;
	return result;
}

/*  pagelen -- query the length of the terminal window, in lines
 *      args: none
 *      rets: the number of lines, or the default for vt100 
 *      if system call failed
 */
int pagelen () {
	int 	termsize [2];
	int	ret_value = DEFAULT_PAGELEN;
	if (get_term_size (termsize) != -1)
		ret_value = termsize [0];
	return (ret_value);
}

/*  pagewidth -- query the width of the terminal window, in characters
 *      args: none
 *      rets: the number of characters, or the default for vt100 
 *      if system call failed
 */
int pagewidth () {
	int 	termsize [2];
	int	ret_value = DEFAULT_PAGEWIDTH;
	if (get_term_size (termsize) != -1)
		ret_value = termsize [1];
	return (ret_value);
}

/*  do_more -- show a page of text, then call how_much_more() for instructions
 *      args: FILE * opened to text to display
 *      rets: SUCCESS if ok, ERROR if not
 */
int do_more( FILE *fp )
{
	int	space_left = pagelen () ;	/* space left on screen */
	int	reply;				/* user request		*/
	FILE	*fp_tty;			/* stream to keyboard	*/

	fp_tty = fopen( CTL_DEV, "r" );		/* connect to keyboard	*/
	while ( has_more_data( fp ) ) {		/* more input	*/
		if ( space_left <= 0 ) {		/* screen full?	*/
			reply = how_much_more(fp_tty);	/* ask user	*/
			if ( reply == 0 )		/*    n: done   */
				break;
			space_left = reply;		/* reset count	*/
		}
		int num_lines_printed = print_one_line( fp );
		space_left -= num_lines_printed;	/* count it	*/
	}
	fclose( fp_tty );			/* disconnect keyboard	*/
	return SUCCESS;				/* EOF => done		*/
}

/*  print_one_line(fp) -- copy data from input to stdout until \n or EOF
 *      args: FILE *  - the file to copy the next line from
 *      rets: the number of screen lines taken up by printing one line from
 *      this file
 */
int print_one_line( FILE *fp )
{
	int	page_width = pagewidth ();	/* will query the device */
	int	num_printed = 0;
	int	c;

	while( ( c = getc(fp) ) != EOF && c != '\n' ) {
		putchar( c ) ;
		num_printed ++;
	}
	putchar('\n');
	return ((num_printed / page_width) + 1);
}

/*  how_much_more -- ask user how much more to show
 *      args: none
 *      rets: number of additional lines to show: 0 => all done
 *	note: space => screenful, 'q' => quit, '\n' => one line
 */
int how_much_more(FILE *fp)
{
	int	c;

	printf("\033[7m more? \033[m");		/* reverse on a vt100	*/
	while( (c = rawgetc(fp)) != EOF )	/* get user input	*/
	{
		if ( c == 'q' )			/* q -> N		*/
			return 0;
		if ( c == ' ' )			/* ' ' => next page	*/
			return pagelen ();	/* how many to show	*/
		if ( c == '\n' )		/* Enter key => 1 line	*/
			return 1;		
	}
	return 0;
}
