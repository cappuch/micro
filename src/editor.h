#ifndef EDITOR_H
#define EDITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <ctype.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define TAB_STOP 8
#define QUIT_TIMES 3

typedef struct erow {
    int size;
    int rsize;
    char *chars;
    char *render;
} erow;

typedef struct editorConfig {
    int cx, cy;
    int rx;
    int rowoff;
    int coloff;
    int screenrows;
    int screencols;
    int numrows;
    erow *row;
    int dirty;
    char *filename;
    char statusmsg[80];
    time_t statusmsg_time;
    struct termios orig_termios;
} editorConfig;

void editorInit(editorConfig *E);
void die(const char *s);
void editorOpen(editorConfig *E, char *filename);
void editorSave(editorConfig *E);
void editorInsertChar(editorConfig *E, int c);
void editorDelChar(editorConfig *E);
void editorInsertNewline(editorConfig *E);
void editorFindCallback(editorConfig *E, char *query, int key);

#include "prototypes.h"

#endif
