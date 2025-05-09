#ifndef PROTOTYPES_H
#define PROTOTYPES_H

#include "editor.h"

void editorDrawMessageBar(editorConfig *E);
void editorRefreshScreen(editorConfig *E);
int editorRowCxToRx(erow *row, int cx);
int getWindowSize(int *rows, int *cols);
void editorSetStatusMessage(editorConfig *E, const char *fmt, ...);
void editorDrawStatusBar(editorConfig *E);
void editorScroll(editorConfig *E);
void editorDrawRows(editorConfig *E);

int editorReadKey(void);
void editorProcessKeypress(editorConfig *E);
void editorMoveCursor(editorConfig *E, int key);

#endif
