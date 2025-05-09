#include "editor.h"
#include "terminal.h"

void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

static void disableRawMode(editorConfig *E) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E->orig_termios) == -1)
        die("tcsetattr");
}

static void enableRawMode(editorConfig *E) {
    if (tcgetattr(STDIN_FILENO, &E->orig_termios) == -1) die("tcgetattr");
    atexit((void(*)(void))disableRawMode);

    struct termios raw = E->orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

static void editorUpdateRow(erow *row) {
    int tabs = 0;
    for (int j = 0; j < row->size; j++)
        if (row->chars[j] == '\t') tabs++;

    free(row->render);
    row->render = malloc(row->size + tabs*(TAB_STOP - 1) + 1);

    int idx = 0;
    for (int j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % TAB_STOP != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}

static void editorInsertRow(editorConfig *E, int at, char *s, size_t len) {
    if (at < 0 || at > E->numrows) return;

    E->row = realloc(E->row, sizeof(erow) * (E->numrows + 1));
    memmove(&E->row[at + 1], &E->row[at], sizeof(erow) * (E->numrows - at));

    E->row[at].size = len;
    E->row[at].chars = malloc(len + 1);
    memcpy(E->row[at].chars, s, len);
    E->row[at].chars[len] = '\0';

    E->row[at].rsize = 0;
    E->row[at].render = NULL;
    editorUpdateRow(&E->row[at]);

    E->numrows++;
    E->dirty++;
}

static void editorFreeRow(erow *row) {
    free(row->render);
    free(row->chars);
}

static void editorDelRow(editorConfig *E, int at) {
    if (at < 0 || at >= E->numrows) return;
    editorFreeRow(&E->row[at]);
    memmove(&E->row[at], &E->row[at + 1], sizeof(erow) * (E->numrows - at - 1));
    E->numrows--;
    E->dirty++;
}

void editorInsertChar(editorConfig *E, int c) {
    if (E->cy == E->numrows) {
        editorInsertRow(E, E->numrows, "", 0);
    }
    erow *row = &E->row[E->cy];
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[E->cx + 1], &row->chars[E->cx], row->size - E->cx + 1);
    row->size++;
    row->chars[E->cx] = c;
    editorUpdateRow(row);
    E->cx++;
    E->dirty++;
}

void editorDelChar(editorConfig *E) {
    if (E->cy == E->numrows) return;
    if (E->cx == 0 && E->cy == 0) return;

    erow *row = &E->row[E->cy];
    if (E->cx > 0) {
        memmove(&row->chars[E->cx - 1], &row->chars[E->cx], row->size - E->cx + 1);
        E->cx--;
        row->size--;
        editorUpdateRow(row);
        E->dirty++;
    } else {
        E->cx = E->row[E->cy - 1].size;
        editorInsertRow(E, E->cy - 1, row->chars, row->size);
        editorDelRow(E, E->cy);
        E->cy--;
    }
}

void editorInsertNewline(editorConfig *E) {
    if (E->cx == 0) {
        editorInsertRow(E, E->cy, "", 0);
    } else {
        erow *row = &E->row[E->cy];
        editorInsertRow(E, E->cy + 1, &row->chars[E->cx], row->size - E->cx);
        row = &E->row[E->cy];
        row->size = E->cx;
        row->chars[row->size] = '\0';
        editorUpdateRow(row);
    }
    E->cy++;
    E->cx = 0;
}

void editorOpen(editorConfig *E, char *filename) {
    free(E->filename);
    E->filename = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        char prompt[80];
        snprintf(prompt, sizeof(prompt), "File '%s' does not exist. Create it? (y/n)", filename);
        
        write(STDOUT_FILENO, "\x1b[s", 3);  // store cursor position and attributes
        
        int modal_row = E->screenrows / 2 - 1;
        char modal_box[256];
        snprintf(modal_box, sizeof(modal_box),
            "\x1b[%d;1H\x1b[7m%*s\x1b[m\n"
            "\x1b[%d;1H\x1b[7m %s \x1b[m\n"
            "\x1b[%d;1H\x1b[7m%*s\x1b[m",
            modal_row, E->screencols, "",
            modal_row + 1, prompt,
            modal_row + 2, E->screencols, "");
        write(STDOUT_FILENO, modal_box, strlen(modal_box));
        
        int c = editorReadKey();
        
        write(STDOUT_FILENO, "\x1b[u", 3);  // restore cursor position and attributes
        editorRefreshScreen(E);
        
        if (c != 'y' && c != 'Y') {
            editorSetStatusMessage(E, "Open aborted");
            free(E->filename);
            E->filename = NULL;
            return;
        }
        fp = fopen(filename, "w+");
        if (!fp) die("fopen");
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;
        editorInsertRow(E, E->numrows, line, linelen);
    }
    free(line);
    fclose(fp);
    E->dirty = 0;
}

static char *editorPromptFilename(editorConfig *E, const char *prompt) {
    char *filename = malloc(128);
    size_t bufsize = 128;
    size_t buflen = 0;

    editorSetStatusMessage(E, prompt);
    editorRefreshScreen(E);

    while (1) {
        int c = editorReadKey();
        if (c == '\r' || c == '\n') {
            if (buflen != 0) {
                editorSetStatusMessage(E, "");
                filename[buflen] = '\0';
                return filename;
            }
        } else if (c == CTRL_KEY('x')) {
            editorSetStatusMessage(E, "");
            free(filename);
            return NULL;
        } else if (c == BACKSPACE || c == CTRL_KEY('h') || c == DEL_KEY) {
            if (buflen > 0) {
                buflen--;
                filename[buflen] = '\0';
                editorSetStatusMessage(E, "%s%s", prompt, filename);
                editorRefreshScreen(E);
            }
        } else if (!iscntrl(c) && c < 128) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                filename = realloc(filename, bufsize);
            }
            filename[buflen++] = c;
            editorSetStatusMessage(E, "%s%s", prompt, filename);
            editorRefreshScreen(E);
        }
    }
}

void editorSave(editorConfig *E) {
    if (E->filename == NULL) {
        char *filename = editorPromptFilename(E, "Save as: ");
        if (filename == NULL) {
            editorSetStatusMessage(E, "Save aborted");
            return;
        }
        E->filename = filename;
    }

    int len = 0;
    char *buf = NULL;
    for (int j = 0; j < E->numrows; j++)
        len += E->row[j].size + 1;

    buf = malloc(len);
    char *p = buf;
    for (int j = 0; j < E->numrows; j++) {
        memcpy(p, E->row[j].chars, E->row[j].size);
        p += E->row[j].size;
        *p = '\n';
        p++;
    }

    FILE *fp = fopen(E->filename, "w");
    if (fp != NULL) {
        fwrite(buf, 1, len, fp);
        fclose(fp);
        E->dirty = 0;
    }
    free(buf);
}

void editorInit(editorConfig *E) {
    E->cx = 0;
    E->cy = 0;
    E->rx = 0;
    E->rowoff = 0;
    E->coloff = 0;
    E->numrows = 0;
    E->row = NULL;
    E->dirty = 0;
    E->filename = NULL;
    E->statusmsg[0] = '\0';
    E->statusmsg_time = 0;

    if (getWindowSize(&E->screenrows, &E->screencols) == -1) die("getWindowSize");
    E->screenrows -= 2;

    enableRawMode(E);
}
