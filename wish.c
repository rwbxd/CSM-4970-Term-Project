#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>

// Global variable definition
const char* pathFilename = "wishPATH";

// Function predeclaration
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

    FILE* pathFile;
    char* pathLine = NULL;
    char* potentialPathLine = NULL;
    size_t pathlen = 0;
    ssize_t pathRead;
    bool pathFlag = false;

    while (1) {
        fprintf(stdout, "wish> ");
        nread = getline(&line, &len, stdin);
        if (nread == -1) {
            returnCode = -1;
            break;
        }

        char* tmpArgs[8]; // TODO: Dynamically allocate args array
        char* token;
        char* saveptr;
        const char* WHITESPACE = " \n";
        for (int j = 0; ; j++, line = NULL) {
            token = strtok_r(line, WHITESPACE, &saveptr);
            if (token == NULL) break;
            tmpArgs[j] = token;
        }
        int argc = 0;
        for (int i = 0; i < 8 && tmpArgs[i] != NULL; i++) argc++;

        char* args[argc + 1];
        for (int i = 0; i < argc; i++) args[i] = tmpArgs[i];
        args[argc] = NULL;

        char* COMMAND = args[0];

        // Check for built-in commands
        if (strcmp(COMMAND, "exit") == 0) {
            break;
        }

        // Check path for command
        //if (pathFile == NULL) {
            pathFile = fopen(pathFilename, "r"); // open pathFile
            if (pathFile == NULL) { break; } // exit if it didn't open
        //}

        while ((pathRead = getline(&pathLine, &pathlen, pathFile)) != -1) {
            potentialPathLine = realloc(potentialPathLine, strlen(pathLine) + strlen(COMMAND) + 2); // +2 for "/" and "/0"
            strcpy(potentialPathLine, pathLine);
            potentialPathLine = strcat(potentialPathLine, "/");
            potentialPathLine = strcat(potentialPathLine, COMMAND);


            if (access(potentialPathLine, X_OK) != -1) {
                pathFlag = true;
                break;
            }
        }

        if (!pathFlag) {
            fprintf(stdout, "Command not found.\n");
            continue; // reset shell
        } else {
            args[0] = potentialPathLine;
        }

        int childPID = fork();
        if (childPID == 0) {
            if (execv(args[0], args) == -1) {
                fprintf(stdout, "Error in execution :(\n");
            }
            exit(1);
        } else {
            int status;
            waitpid(childPID, &status, 0);
        }
    }

    if (pathFile != NULL) fclose(pathFile);
    free(line);
    free(pathLine);
    free(potentialPathLine);
    return returnCode;
}