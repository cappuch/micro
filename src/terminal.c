#include "editor.h"
#include <errno.h>
#include "terminal.h"
#include <sys/ioctl.h>
#include <stdarg.h>

#include "editor.h"
#include <errno.h>


void editorDrawMessageBar(editorConfig *E) {
    write(STDOUT_FILENO, "\x1b[K", 3);
    int msglen = strlen(E->statusmsg);
    if (msglen > E->screencols) msglen = E->screencols;
    if (msglen && time(NULL) - E->statusmsg_time < 5)
        write(STDOUT_FILENO, E->statusmsg, msglen);
}

void editorRefreshScreen(editorConfig *E) {
    editorScroll(E);
    write(STDOUT_FILENO, "\x1b[?25l", 6);
    write(STDOUT_FILENO, "\x1b[H", 3);
    editorDrawRows(E);
    editorDrawStatusBar(E);
    editorDrawMessageBar(E);
    
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", 
        (E->cy - E->rowoff) + 1, 
        (E->rx - E->coloff) + 1);
    write(STDOUT_FILENO, buf, strlen(buf));
    
    write(STDOUT_FILENO, "\x1b[?25h", 6);
}

int editorRowCxToRx(erow *row, int cx) {
    int rx = 0;
    for (int j = 0; j < cx; j++) {
        if (row->chars[j] == '\t')
            rx += (TAB_STOP - 1) - (rx % TAB_STOP);
        rx++;
    }
    return rx;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return -1;
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

void editorSetStatusMessage(editorConfig *E, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E->statusmsg, sizeof(E->statusmsg), fmt, ap);
    va_end(ap);
    E->statusmsg_time = time(NULL);
}

void editorDrawStatusBar(editorConfig *E) {
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
        E->filename ? E->filename : "[No Name]", E->numrows,
        E->dirty ? "(modified)" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
        E->cy + 1, E->numrows);

    if (len > E->screencols) len = E->screencols;
    write(STDOUT_FILENO, "\x1b[7m", 4);
    write(STDOUT_FILENO, status, len);
    while (len < E->screencols) {
        if (E->screencols - len == rlen) {
            write(STDOUT_FILENO, rstatus, rlen);
            break;
        } else {
            write(STDOUT_FILENO, " ", 1);
            len++;
        }
    }
    write(STDOUT_FILENO, "\x1b[m", 3);
    write(STDOUT_FILENO, "\r\n", 2);
}

void editorScroll(editorConfig *E) {
    E->rx = 0;
    if (E->cy < E->numrows) {
        E->rx = editorRowCxToRx(&E->row[E->cy], E->cx);
    }

    if (E->cy < E->rowoff) {
        E->rowoff = E->cy;
    }
    if (E->cy >= E->rowoff + E->screenrows) {
        E->rowoff = E->cy - E->screenrows + 1;
    }
    if (E->rx < E->coloff) {
        E->coloff = E->rx;
    }
    if (E->rx >= E->coloff + E->screencols) {
        E->coloff = E->rx - E->screencols + 1;
    }
}

void editorDrawRows(editorConfig *E) {
    for (int y = 0; y < E->screenrows; y++) {
        int filerow = y + E->rowoff;
        if (filerow >= E->numrows) {
            if (E->numrows == 0 && y == E->screenrows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                    "Nano editor clone -- version %s", "0.0.1");
                if (welcomelen > E->screencols) welcomelen = E->screencols;
                int padding = (E->screencols - welcomelen) / 2;
                if (padding) {
                    write(STDOUT_FILENO, "~", 1);
                    padding--;
                }
                while (padding--) write(STDOUT_FILENO, " ", 1);
                write(STDOUT_FILENO, welcome, welcomelen);
            } else {
                write(STDOUT_FILENO, "~", 1);
            }
        } else {
            int len = E->row[filerow].rsize - E->coloff;
            if (len < 0) len = 0;
            if (len > E->screencols) len = E->screencols;
            write(STDOUT_FILENO, &E->row[filerow].render[E->coloff], len);
        }

        write(STDOUT_FILENO, "\x1b[K", 3);
        write(STDOUT_FILENO, "\r\n", 2);
    }
}
