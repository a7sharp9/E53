/*
 * ball.h
 *
 *  Created on: Mar 11, 2018
 *      Author: Yuri
 */

#ifndef BALL_H
#define BALL_H

#include "boundaries.h"

void ball_init (struct boundaries const *boundaries);
int ball_bounce ();
void ball_move ();

void bounce_or_lose ();

void ticker_set ();
void ticker_quit ();

void format_elapsed (char *fmttime);
void print_elapsed_seconds ();
#define MAXTIMELEN 6 // the time string will be no longer than this
#define MAXTIME (60 * 60) // will turn back to 0 after that

#endif /* BALL_H */
