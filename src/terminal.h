#ifndef TERMINAL_H
#define TERMINAL_H

#include "editor.h"

#define ARROW_LEFT 1000
#define ARROW_RIGHT 1001
#define ARROW_UP 1002
#define ARROW_DOWN 1003
#define DEL_KEY 1004
#define HOME_KEY 1005
#define END_KEY 1006
#define PAGE_UP 1007
#define PAGE_DOWN 1008
#define BACKSPACE 127

int getWindowSize(int *rows, int *cols);
int editorReadKey(void);
void editorSetStatusMessage(editorConfig *E, const char *fmt, ...);
void editorDrawStatusBar(editorConfig *E);
void editorScroll(editorConfig *E);
void editorDrawRows(editorConfig *E);

#endif
