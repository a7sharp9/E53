#include	<stdio.h>
#include	<string.h>
#include	<sys/types.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<utmp.h>

void dumpfile(const char *fn);
void show_utrec( const struct utmp *rp );

/*
 * dumputmp.c
 *   purpose
 *      reads from any file containing utmp records and prints
 *      the records in text format to standard output
 *   usage
 *      dumputmp [filename|-size]
 *   action
 *      reads from default utmp file or from named file
 *      if argument is "-size", it prints the size of a utmp record
 */

int main(int ac, char **av)
{
	if ( ac == 1 )
		dumpfile( UTMP_FILE );
	else
		dumpfile( av[1] );

	return 0;
}

void
dumpfile( const char *fn )
/*
 * open file and dump records
 */
{
	struct utmp	utrec;
	int		fd;

	if ( strcmp(fn,"-size") == 0 ){
                printf("%ld\n", (long)sizeof(utrec));
		return;
	}
	fd = open( fn, O_RDONLY );
	if ( fd == -1 )
	{
		perror( fn );
		return ;
	}
	while( read( fd, &utrec, sizeof(utrec) ) == sizeof(utrec) )
		show_utrec(&utrec);
	close( fd );
}
	
void
show_utrec( const struct utmp *rp )
{
	char	*typename(int typenum);
	char ut_id_fmt[8];
	sprintf(ut_id_fmt, "%%-8.%lds ", (long)sizeof(rp->ut_id));

	printf("%-8.8s ", rp->ut_user );
	printf(ut_id_fmt, rp->ut_id   );
	printf("%-12.12s ", rp->ut_line );
	printf("%6d ", rp->ut_pid );
	printf("%4d %-12.12s ", rp->ut_type , typename(rp->ut_type) );
	printf("%12ld ", (long)rp->ut_time );
	printf("%s ", rp->ut_host );
	putchar('\n');
}

char *uttypes[] = {  	"EMPTY", "RUN_LVL", "BOOT_TIME", "OLD_TIME", 
			"NEW_TIME", "INIT_PROCESS", "LOGIN_PROCESS", 
			"USER_PROCESS", "DEAD_PROCESS", "ACCOUNTING"
	};

char *
typename( int typenum )
{
	return uttypes[typenum];
}
