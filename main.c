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
    int id_custom_command;
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

Command parseCommand(char *line) {

    int nr_args = 0;
    char *str[MAX_NR_ARGS];
    char *ptr;
    char sep[] = " ";
    ptr = strtok(line,sep);
    while(ptr != NULL){
        str[nr_args] = malloc(strlen(ptr) + 1);
        strcpy(str[nr_args], ptr);

        ptr = strtok(NULL,sep);
        nr_args++;
    }
    //TODO CHECK

    Command currCommand;
    strcpy(currCommand.raw_command,str[0]);

    currCommand.nr_args = nr_args;

    for(int i = 0; i < nr_args; i++){
        currCommand.arguments[i] = malloc(strlen(str[i] ) + 1);
        strcpy(currCommand.arguments[i],str[i]);
        free(str[i]);
    }
    currCommand.arguments[nr_args] = NULL;
    currCommand.commandType = SYSTEM;
    for(int i = 0; i < NUM_COMMANDS; i++){
        if(strcmp(custom_commands[i],currCommand.raw_command) == 0){
            currCommand.commandType = CUSTOM;
            currCommand.id_custom_command = i;
            break;
        }
    }


    return currCommand;
}

int executeCustomCommand(Command currCommand){
    switch (currCommand.id_custom_command){
        case 0: {
            printf("trying to clear\n");
            break;
        }
        case 1:{
            printf("trying to cd\n");
            break;
        }

        default: {
            printf("Unknown command\n");
            return -1;
        }
    }
    return 0;
}

int executeSystemCommand(Command cmd) {

    pid_t worker = fork();
    if (worker < 0) {
        printf("Error: Unable to fork child \n\n");
        return -1;
    }
    if (worker == 0) {
        execvp(cmd.raw_command, cmd.arguments );
    } else {
        int status = 0;
        wait(&status);
        return status;
    }

}

int executeCommand(Command currCommand){
    if(currCommand.commandType == CUSTOM){
        return executeCustomCommand(currCommand);

    }
    if (currCommand.commandType == SYSTEM) {
        return executeSystemCommand(currCommand);
    }

    return -1;
}


int main() {

    updateCurrentDir();

    while ( 1 ) {
        char* line = readInput();
        executeCommand(parseCommand(line));
    }


    return 0;
}
#pragma clang diagnostic pop