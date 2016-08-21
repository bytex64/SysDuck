/*
 * terminal.h
 *
 * Created: 12/13/2015 0:24:56
 *  Author: chip
 */ 


#ifndef TERMINAL_H_
#define TERMINAL_H_

void term_init(void);
int term_readline(char *buf, int n);
void term_goto(int x, int y);
void term_clear(void);

#endif /* TERMINAL_H_ */