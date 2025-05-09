#include "editor.h"
#include "terminal.h"

static void editorMoveCursor(editorConfig *E, int key) {
    erow *row = (E->cy >= E->numrows) ? NULL : &E->row[E->cy];

    switch (key) {
        case 'h':
        case ARROW_LEFT:
            if (E->cx != 0) {
                E->cx--;
            } else if (E->cy > 0) {
                E->cy--;
                E->cx = E->row[E->cy].size;
            }
            break;
        case 'l':
        case ARROW_RIGHT:
            if (row && E->cx < row->size) {
                E->cx++;
            } else if (row && E->cx == row->size) {
                E->cy++;
                E->cx = 0;
            }
            break;
        case 'k':
        case ARROW_UP:
            if (E->cy != 0) E->cy--;
            break;
        case 'j':
        case ARROW_DOWN:
            if (E->cy < E->numrows) E->cy++;
            break;
    }

    row = (E->cy >= E->numrows) ? NULL : &E->row[E->cy];
    int rowlen = row ? row->size : 0;
    if (E->cx > rowlen) E->cx = rowlen;
}

void editorProcessKeypress(editorConfig *E) {
    static int quit_times = QUIT_TIMES;
    int c = editorReadKey();

    switch (c) {
        case CTRL_KEY('x'):
            if (E->dirty && quit_times > 0) {
                editorSetStatusMessage(E, "WARNING!!! File has unsaved changes. "
                    "Press Ctrl-X %d more times to quit.", quit_times);
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

        case '\r':
            editorInsertNewline(E);
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

int main(int argc, char *argv[]) {
    editorConfig E;
    editorInit(&E);

    if (argc >= 2) {
        editorOpen(&E, argv[1]);
    }

    editorSetStatusMessage(&E, "HELP: Ctrl-S = save | Ctrl-X = quit");

    while (1) {
        editorRefreshScreen(&E);
        editorProcessKeypress(&E);
    }

    return 0;
}
