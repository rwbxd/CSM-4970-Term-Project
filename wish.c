#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>

// Global variable definition
char* pathFilename;

// Function predeclaration
int setupPath();
int interactiveLoop();

int main(int argc, char* argv[]) {
    if (setupPath() == -1) {
        fprintf(stdout, "Error getting path ready. Things will likely not work right!\n");
    }
    if (argc == 1) {
        exit(interactiveLoop());
    }
    free(pathFilename);
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
            fprintf(stdout, "Failed to get input line, exiting.\n");
            break;
        }

        char* tmpArgs[8]; // TODO: Dynamically allocate args array
        char* token;
        char* saveptr;
        const char* WHITESPACE = " \n";
        for (int tmpArg = 0; tmpArg < 8; tmpArg++) tmpArgs[tmpArg] = NULL;
        for (int j = 0; ; j++, line = NULL) { // while loop in disguise
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
        } else if (strcmp(COMMAND, "cd") == 0) {
            if (argc == 1) fprintf(stdout, "cd: requires a directory (cd dir)\n");
            else if (argc > 2) fprintf(stdout, "cd: too many arguments\n");
            else {
                if (chdir(args[1]) != 0) { // failure
                    fprintf(stdout, "cd: chdir failed\n");
                }
            }
            continue; // reset shell
        }

        // Check path for command
        //if (pathFile == NULL) {
            pathFile = fopen(pathFilename, "r"); // open pathFile
            if (pathFile == NULL) {
                fprintf(stdout, "Failed to open pathfile.\n");
                break; // exit if it didn't open
            } 
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

int setupPath() {
    // https://pubs.opengroup.org/onlinepubs/9699919799/functions/getcwd.html
    size_t size; // size of path
    char* pathFile = "/wishPATH";

    // set initial path size
    long path_max = pathconf(".", _PC_PATH_MAX);
    if (path_max == -1) size = 1024;
    else if (path_max > 10240) size = 10240;
    else size = path_max;

    char* buf;
    char* ptr;
    for (buf = ptr = NULL; ptr == NULL; size *= 2) { // Double size until we don't get not enough memory error
        if ((buf = realloc(buf, size + strlen(pathFile))) == NULL) {
            return 1; // error
        }

        ptr = getcwd(buf, size - strlen(pathFile));
        if (ptr == NULL && errno != ERANGE) {
            return 1; // error
        }
    }

    strcat(buf, pathFile);
    pathFilename = buf;
}