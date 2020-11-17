#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sys/param.h>
#include "read.h"

/*
 * readline -- read in a line from fp, stop at \n
 *    args: buf - place to store line
 *          len - size of buffer
 *          fp  - input stream
 *    rets: NULL at EOF else the buffer
 *    note: will not overflow buffer, but will read until \n or EOF
 *          thus will lose data if line exceeds len-2 chars
 *    note: like fgets but will always read until \n even if it loses data
 */
char *readline (char *buf, int len, FILE *fp) {
	int space = len - 2;
	char *cp = buf;
	int c;

	while ( (c = getc(fp)) != '\n' && c != EOF) {
		if (space-- > 0)
			*cp++ = c;
	}
	if (c == '\n')
		*cp++ = c;
	*cp = '\0';
	return (c == EOF && cp == buf ? NULL : buf);
}

char *
full_hostname ()
/*
 * returns full `official' hostname for current machine
 * NOTE: this returns a ptr to a static buffer that is
 *       overwritten with each call. ( you know what to do.)
 */
{
	struct hostent *hp;
	char hname[MAXHOSTNAMELEN];
	static char fullname[MAXHOSTNAMELEN];

	if (gethostname (hname, MAXHOSTNAMELEN) == -1) /* get rel name	*/
	{
		perror ("gethostname");
		exit (1);
	}
	hp = gethostbyname (hname); /* get info about host	*/
	if (hp == NULL) /*   or die		*/
		return NULL;
	strcpy (fullname, hp->h_name); /* store foo.bar.com	*/
	return fullname; /* and return it	*/
}

void read_til_crnl (FILE *fp) {
	char buf[MAX_RQ_LEN];
	while (readline (buf, MAX_RQ_LEN, fp) != NULL && strcmp (buf, "\r\n") != 0)
		;
}

/*
 * read the http request into rq not to exceed rqlen
 * return -1 for error, 0 for success
 */
int read_request (FILE *fp, char rq[], int rqlen) {
	/* null means EOF or error. Either way there is no request */
	if (readline (rq, rqlen, fp) == NULL)
		return -1;
	read_til_crnl (fp);
	return 0;
}

