#include "input.h"
#include "terminal.h"
#include <errno.h>

int editorReadKey(void) {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }

        return '\x1b';
    } else {
        return c;
    }
}

void editorProcessKeypress(editorConfig *E) {
    static int quit_times = QUIT_TIMES;
    
    int c = editorReadKey();

    switch (c) {
        case '\r':
            editorInsertNewline(E);
            break;

        case CTRL_KEY('q'):
        case CTRL_KEY('x'):
            if (E->dirty && quit_times > 0) {
                editorSetStatusMessage(E, "WARNING!!! File has unsaved changes. "
                    "Press Ctrl-Q or Ctrl-X %d more times to quit.", quit_times);
                quit_times--;
                return;
            }
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case CTRL_KEY('s'):
            editorSave(E);
            break;

        case HOME_KEY:
            E->cx = 0;
            break;
        case END_KEY:
            if (E->cy < E->numrows)
                E->cx = E->row[E->cy].size;
            break;

        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY) editorMoveCursor(E, ARROW_RIGHT);
            editorDelChar(E);
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                if (c == PAGE_UP) {
                    E->cy = E->rowoff;
                } else if (c == PAGE_DOWN) {
                    E->cy = E->rowoff + E->screenrows - 1;
                    if (E->cy > E->numrows) E->cy = E->numrows;
                }

                int times = E->screenrows;
                while (times--)
                    editorMoveCursor(E, c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(E, c);
            break;

        case CTRL_KEY('l'):
        case '\x1b':
            break;

        default:
            editorInsertChar(E, c);
            break;
    }

    quit_times = QUIT_TIMES;
}

void editorMoveCursor(editorConfig *E, int key) {
    erow *row = (E->cy >= E->numrows) ? NULL : &E->row[E->cy];

    switch (key) {
        case ARROW_LEFT:
            if (E->cx != 0) {
                E->cx--;
            } else if (E->cy > 0) {
                E->cy--;
                E->cx = E->row[E->cy].size;
            }
            break;
        case ARROW_RIGHT:
            if (row && E->cx < row->size) {
                E->cx++;
            } else if (row && E->cx == row->size) {
                E->cy++;
                E->cx = 0;
            }
            break;
        case ARROW_UP:
            if (E->cy != 0) {
                E->cy--;
            }
            break;
        case ARROW_DOWN:
            if (E->cy < E->numrows) {
                E->cy++;
            }
            break;
    }

    row = (E->cy >= E->numrows) ? NULL : &E->row[E->cy];
    int rowlen = row ? row->size : 0;
    if (E->cx > rowlen) {
        E->cx = rowlen;
    }
}
