/*  more02.c  - version 0.2 of more
 *	read and print 24 lines then pause for a few special commands
 *      feature of version 0.2: reads from /dev/tty for commands
 */
#include	<stdio.h>
#include	<stdlib.h>

#define	PAGELEN	24
#define	LINELEN	512
#define	ERROR	1
#define	SUCCESS	0
#define	CTL_DEV	"/dev/tty"		/* source of control commands	*/

int do_more(FILE *);
int how_much_more(FILE *);

int main( int ac , char *av[] )
{
	FILE	*fp;			/* stream to view with more	*/
	int	result = SUCCESS;	/* return status from main	*/

	if ( ac == 1 )
		result = do_more( stdin );
	else
		while ( result == SUCCESS && --ac )
			if ( (fp = fopen( *++av , "r" )) != NULL )
			{
				result = do_more( fp ) ; 
				fclose( fp );
			}
			else
				result = ERROR;
	return result;
}

/*  do_more -- show a page of text, then call how_much_more() for instructions
 *      args: stream to file to display
 *      rets: SUCCESS if ok, ERROR if not
 */
int do_more( FILE *fp )
{
	char	line[LINELEN];
	int	space_left = PAGELEN ;		/* space left on screen */
	int	reply, retval = SUCCESS;	/* user request		*/
	FILE	*fp_tty;			/* read cmds from here	*/

	fp_tty = fopen( CTL_DEV, "r" );	   	/* NEW: cmd stream   */
	if ( fp_tty == NULL )			   /* if open fails     */
		return ERROR;			   /* tell caller	*/

	while ( fgets( line, LINELEN, fp ) != NULL ){	/* more input	*/
		if ( space_left <= 0 ) {		/* screen full?	*/
			reply = how_much_more(fp_tty);	/* ask user	*/
			if ( reply == 0 )		/*    n: done   */
				break;
			space_left = reply;		/* reset count	*/
		}
		if ( fputs( line, stdout )  == EOF ){	/* show line	*/
			retval = ERROR;
			break;
		}
		space_left--;				/* count it	*/
	}
	fclose( fp_tty );				/* 10Q: alexis	*/
	return retval;					/* EOF => done	*/
}
/*  how_much_more -- ask user how much more to show
 *      args: none
 *      rets: number of additional lines to show: 0 => all done
 *	note: space => screenful, 'q' => quit, '\n' => one line
 */
int how_much_more(FILE *cmd)			   /* NEW: accepts arg  */
{
	int	c;

	printf("\033[7m more? \033[m");		/* reverse on a vt100	*/
	while( (c = getc(cmd)) != EOF )		/* NEW: reads from tty  */
	{
		if ( c == 'q' )			/* q -> N		*/
			return 0;
		if ( c == ' ' )			/* ' ' => next page	*/
			return PAGELEN;		/* how many to show	*/
		if ( c == '\n' )		/* Enter key => 1 line	*/
			return 1;		
	}
	return 0;
}
