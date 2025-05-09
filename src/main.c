#include "editor.h"
#include "terminal.h"

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
