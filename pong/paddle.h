/*
 * paddle.h
 *
 *  Created on: Mar 11, 2018
 *      Author: Yuri
 */

#ifndef PADDLE_H
#define PADDLE_H

#include "boundaries.h"

void paddle_up ();
void paddle_down ();
void paddle_init (struct boundaries const *boundaries);
int paddle_contact (int y, int x);

#endif /* PADDLE_H_ */
