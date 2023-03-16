#include <stdlib.h>
#include <stdio.h>

int interactiveLoop();

int main(int argc, char* argv[]) {
    if (argc == 1) {
        exit(interactiveLoop());
    }
}

int interactiveLoop() {
    char* line = NULL;
    size_t len = 0;
    ssize_t nread;
    int returnCode = 0;

    while (1) {
        fprintf(stdout, "wish> ");
        nread = getline(&line, &len, stdin);
        if (nread == -1) {
            returnCode = -1;
            break;
        }

        // Check path for command
        


        fprintf(stdout, "%s", line);
    }

    free(line);
    return returnCode;
}