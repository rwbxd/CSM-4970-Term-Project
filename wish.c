#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>

// Path linked list
struct pathEntry {
    char* path;
    size_t len;
    struct pathEntry* next;
};

// Args linked list
struct argEntry {
    char* arg;
    struct argEntry* next;
};

// Global variable definition
struct pathEntry* pathHead = NULL;
struct argEntry* argHead = NULL;

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
}

int interactiveLoop() {
    char* line = NULL;
    size_t len = 0;
    ssize_t nread;
    int returnCode = 0;

    while (1) { // main loop
        fprintf(stdout, "wish> ");
        nread = getline(&line, &len, stdin); // get input line, write to line var
        if (nread == -1) {
            returnCode = -1;
            fprintf(stdout, "Failed to get input line, exiting.\n");
            break;
        }

        while (argHead != NULL) { // clear previous args    
            struct argEntry* next = argHead->next; 
            free(argHead);
            argHead = next;
        }

        struct argEntry* curArg = (struct argEntry*) malloc(sizeof(struct argEntry));
        argHead = curArg;
        char* token;
        char* saveptr;
        const char* WHITESPACE = " \n";
        int argc = -1;
        do {
            argc++;
            // TODO: Fix wasted entry when token is null?
            token = strtok_r(line, WHITESPACE, &saveptr); // get a token from the line, delimited by whitespace
            curArg->arg = token;
            curArg->next = (struct argEntry*) malloc(sizeof(struct argEntry));
            curArg = curArg->next;
            curArg->next = NULL;
            line = NULL;
        } while (token != NULL);

        // flatten linked list of variable size to array for use with execv
        char* args[argc + 1];
        curArg = argHead;
        for (int i = 0; i < argc; i++) {
            args[i] = curArg->arg;
            curArg = curArg->next;
        };
        args[argc] = NULL;

        char* COMMAND = args[0];

        // Check for built-in commands
        if (strcmp(COMMAND, "exit") == 0) {
            break;
        } else if (strcmp(COMMAND, "cd") == 0) { // TODO: dir in quotes
            if (argc == 1) fprintf(stdout, "cd: requires a directory (cd dir)\n");
            else if (argc > 2) fprintf(stdout, "cd: too many arguments\n");
            else {
                if (chdir(args[1]) != 0) { // failure
                    fprintf(stdout, "cd: chdir failed\n");
                }
            }
            continue; // reset shell
        } else if (strcmp(COMMAND, "path") ==  0) {
            while (pathHead != NULL) {
                struct pathEntry* next = pathHead->next;
                free(pathHead);
                pathHead = next;
            }
            struct pathEntry* cur;
            if (argc > 1) {
                cur = (struct pathEntry*) malloc(sizeof(struct pathEntry));
                cur->path = args[1];
                cur->len = strlen(cur->path);
                pathHead = cur;
            }
            for (int arg = 2; arg < argc; arg++) {
                cur->next = (struct pathEntry*) malloc(sizeof(struct pathEntry));
                cur = cur->next;
                cur->path = args[arg];
                cur->len = strlen(cur->path);
                cur->next = NULL;
            }
            continue;
        }

        // else, try finding a command in path
        char* potentialPathLine = NULL;
        struct pathEntry* curPathEntry = pathHead;
        while (curPathEntry != NULL) {
            potentialPathLine = realloc(potentialPathLine, curPathEntry->len + strlen(COMMAND) + 2); // +2 for "/" and "/0"
            strcpy(potentialPathLine, curPathEntry->path);
            potentialPathLine = strcat(potentialPathLine, "/");
            potentialPathLine = strcat(potentialPathLine, COMMAND);

            if (access(potentialPathLine, X_OK) != -1) { break; } // found path
            else { curPathEntry = curPathEntry->next; } // didn't find path, try next entry
        }

        if (curPathEntry == NULL) { // exhausted all entries
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

    free(line);
    //free(potentialPathLine);
    return returnCode;
}

int setupPath() {
    FILE* pathFile = fopen("wishPATH", "r"); // open pathFile
    if (pathFile == NULL) {
        fprintf(stdout, "Failed to open pathfile.\n");
        return -1; // exit if it didn't open
    }

    char* pathLine = NULL;
    char* potentialPathLine = NULL;
    size_t pathlen = 0;
    ssize_t pathRead;

    struct pathEntry* cur;
    while ((pathRead = getline(&pathLine, &pathlen, pathFile)) != -1) {
        struct pathEntry* new = (struct pathEntry*) malloc(sizeof(struct pathEntry));
        new->path = pathLine;
        new->len = strlen(new->path);
        if (pathHead == NULL) {
            pathHead = new;
            cur = new;
        } else {
            cur->next = new;
            cur = cur->next;
        }
    }

    fclose(pathFile);
    return 0;
}