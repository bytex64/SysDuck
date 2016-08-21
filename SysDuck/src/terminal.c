#include "terminal.h"
#include <stdio.h>
#include <asf.h>

#define CSI "\x1b["

void term_init() {
	delay_s(1);
}

int term_readline(char *buf, int n) {
	int i = 0;
	
	while (i < n - 1) {
		int c = getchar();
		if (c == '\r' || c == '\n') {
			putchar('\r');
			putchar('\n');
			buf[i] = 0;
			break;
		} else if (c == 0x08 || c == 0x7f) {
			if (i > 0) {
				printf("\x08 \x08");
				i--;
				buf[i] = 0;
			}
			continue;
		}
		putchar(c);
		buf[i] = (char)c;
		i++;
	}
	if (i == n - 1) {
		buf[n - 1] = 0;
	}
	return i;
}


void term_goto(int x, int y) {
	printf(CSI "%d;%dH", y, x);
}

void term_clear() {
	printf(CSI "2J");
}