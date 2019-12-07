#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<readline/readline.h>
#include<readline/history.h>
#include <pwd.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

#define BUFF_SIZE (1024)
#define NUM_COMMANDS (2)
#define MAX_NR_ARGS (10)

enum Command_Type {
    CUSTOM, SYSTEM
};

typedef struct  {
    char raw_command[BUFF_SIZE];
    char *arguments[MAX_NR_ARGS];
    int nr_args;
    enum Command_Type commandType;

}Command;

char *currentDir[BUFF_SIZE];

char *custom_commands[NUM_COMMANDS] = {
        "clear",
        "cd"
};

const char *getUserName()
{
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw)
    {
        return pw->pw_name;
    }

    return "";
}

void updateCurrentDir() {
    getcwd(currentDir, BUFF_SIZE);
}

char *readInput() {
    char *prompt[BUFF_SIZE];
    sprintf(prompt, "%s:%s: ", getUserName(),  currentDir);
    char* line = readline(prompt);
    return line;
}

int isCustomCommand(Command command) {
    for (int i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(command, custom_commands[i]) == 0) return i;
    }

    return -1;
}

int executeCustomCommand(int commandNr) {
    switch (commandNr) {
        case 0: {
            printf("tryig to execute clear\n");
            break;
        }
        case 1: {
            printf("trying to execute hello\n");
            break;
        }

        default: {
            printf("Error: Unknown command");
        }
    }
}

int interpretCommand(char* command) {
    int commandNr = isCustomCommand(command);
    if (commandNr != -1) {
        executeCustomCommand(commandNr);
        return 0;
    }

    return -1;
}

void parseCommand() {

}

int main() {

    updateCurrentDir();

    while ( 1 ) {
        char* line = readInput();
        interpretCommand(line);
    }


    return 0;
}
#pragma clang diagnostic pop