#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<signal.h>
#include	<sys/wait.h>
#include	"smsh.h"
#include	"splitline.h"
#include	"varlib.h"
#include	"process.h"

/**
 **	small-shell version 5
 **		first really useful version after prompting shell
 **		this one parses the command line into strings
 **		uses fork, exec, wait, and ignores signals
 **
 **     hist: 2017-04-12: changed comment to say "version 5"
 **/

void setup()
/*
 * purpose: initialize shell
 * returns: nothing. calls fatal() if trouble
 */
{
	extern char **environ;

	VLenviron2table(environ);
	signal(SIGINT,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
}

void fatal(char *s1, char *s2, int n)
{
	fprintf(stderr,"Error: %s,%s\n", s1, s2);
	exit(n);
}
