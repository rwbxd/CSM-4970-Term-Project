#include <errno.h>
#include <fcntl.h>
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
int redirectionFile = -1;
int argChildPID = 0;
bool isAChild = false;

// Function predeclaration
int setupPath();
int interactiveLoop();
int batchLoop();
int parseAndExecute(char* line);
int executeCommand(char* args[]);

int main(int argc, char* argv[]) {
    if (setupPath() == -1) fprintf(stdout, "Error getting path ready. Things will likely not work right!\n");

    if (argc == 1) {
        exit(interactiveLoop());
    } else {
        exit(batchLoop(argv[1]));
    }
}

int interactiveLoop() {
    char* line = NULL;
    size_t len = 0;
    int nread = -1;
    int returnCode;

    while (isAChild == false) { // main loop -- isAChild is a global variable
        free(line);
        line = NULL;
        fprintf(stdout, "wish> ");
        nread = getline(&line, &len, stdin); // get input line, write to line var
        if (nread == -1) {
            returnCode = -1;
            fprintf(stdout, "Failed to get input line, exiting.\n");
            return -1;
        }

        returnCode = parseAndExecute(line);
        //printf("%d\n", returnCode);
        if (isAChild) return 0;
        if (returnCode != 0) return returnCode; // nonzero return code = exit code or error code
        // else continue to next iteration
    }

    return returnCode;
}

int batchLoop(char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stdout, "Failed to open file.\n");
        return -1; // exit if it didn't open
    }

    char* line = NULL;
    size_t len = 0;
    ssize_t nread;
    int returnCode = 0;

    while ((nread = getline(&line, &len, file)) != -1) { // while lines are read from file
        if (parseAndExecute(line) == -2) break;
    }

    fclose(file);
    if (line) free(line);

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

int parseAndExecute(char* line) {
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
    int argc = 0;
    token = strtok_r(line, WHITESPACE, &saveptr); // get a token from the line, delimited by whitespace
    while (token != NULL) {
        if (strcmp(token, ">") == 0) {
            token = strtok_r(line, WHITESPACE, &saveptr); // get a token from the line, delimited by whitespace
            redirectionFile = open(/*file:*/token, /*options:*/O_RDWR | O_CREAT, /*file perms:*/00777);
            if (redirectionFile == -1) fprintf(stdout, "Failed to open redirection file, stdout will be used.");
            break;
        } else if (strcmp(token, "&") == 0) {
            argChildPID = fork();
            if (argChildPID == 0) { // child
                isAChild = true;
                struct argEntry* curArg = argHead;
                while (curArg != NULL) { // clear args
                    argHead = argHead->next;
                    free(curArg);
                    curArg = argHead;
                }
                curArg = (struct argEntry*) malloc(sizeof(struct argEntry));
                argHead = curArg;
                argc = 0;
                token = strtok_r(line, WHITESPACE, &saveptr); // get a token from the line, delimited by whitespace
                continue; // go back to while(token != NULL), start parsing
            } else { // parent
                break; // run commands held so far
            }
        }

        argc++;
        curArg->arg = token;
        curArg->next = (struct argEntry*) malloc(sizeof(struct argEntry));
        curArg = curArg->next;
        curArg->next = NULL;
        line = NULL;
        token = strtok_r(line, WHITESPACE, &saveptr); // get a token from the line, delimited by whitespace
    }

    if (argc == 0) {
        free(curArg);
        argHead = NULL; // manually clear argHead to prevent double free
        return 0; // reset shell or exit
    }

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
        return 1;
    } else if (strcmp(COMMAND, "cd") == 0) { // TODO: dir in quotes
        if (argc == 1) fprintf(stdout, "cd: requires a directory (cd dir)\n");
        else if (argc > 2) fprintf(stdout, "cd: too many arguments\n");
        else {
            if (chdir(args[1]) != 0) { // failure
                fprintf(stdout, "cd: chdir failed\n");
            }
        }
        return 0; // reset shell or exit
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
        return 0; // reset shell or exit
    }

    char* pathLine = NULL;
    char* potentialPathLine = NULL;
    size_t pathlen = 0;
    ssize_t pathRead;

    // else, try finding a command in path
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
        return 0; // reset shell or exit
    } else {
        args[0] = potentialPathLine;
    }

    executeCommand(args);
    
    if (redirectionFile != -1) close(redirectionFile);
    int status;
    if (argChildPID != 0) waitpid(argChildPID, &status, 0);

    return 0;
}

int executeCommand(char* args[]) {
    int execChildPID = fork();

    if (execChildPID == 0) { // Child
        if (redirectionFile != -1) { // If we're piping to a redirection file
            dup2(redirectionFile, STDOUT_FILENO); // pipe stdout to file
            dup2(redirectionFile, STDERR_FILENO); // pipe stderr to file
            close(redirectionFile); // not needed after duping fp
        }

        int returnCode = execv(args[0], args);
        if (returnCode == -1) fprintf(stdout, "Error in execution :(\n");

        exit(1); // exit child
    } else { // Parent
        int status;
        waitpid(execChildPID, &status, 0);
        return 0;
    }
}