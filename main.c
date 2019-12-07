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
#define NUM_COMMANDS (1)
#define MAX_NR_ARGS (10)
#define MAX_CHAIN_CMD (10)

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

typedef struct{
    Command cmds[MAX_CHAIN_CMD];
    int nr_cmds;
    int op[MAX_CHAIN_CMD-1];
}LogicCommand;

char *currentDir[BUFF_SIZE];

char *custom_commands[NUM_COMMANDS] = {
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

LogicCommand parseLogicCommand(const char *line){
    int cnt = 0;
    char *str[MAX_CHAIN_CMD];
    LogicCommand lgc;

    str[0] = malloc(BUFF_SIZE);
    for(int i = 0; line[i] != '\0'; i++){
        if(line[i] == '&' && line[i + 1] == '&'){
            cnt++;
            str[cnt] = malloc(BUFF_SIZE);
            i++;
        }
    }

    cnt = 0;
    for(int i = 0, idx = 0; line[i] != '\0'; i++, idx++){
        if(line[i] == '&' && line[i + 1] == '&'){
            printf("reached & ad %d\n", i );
            cnt++;
            i++;
            lgc.op[i - 1] = 0;
            idx = 0;
        } else{
            str[cnt][idx] = line[i];
        }
    }


    for(int i = 0;  i <= cnt; i++){
        lgc.cmds[i] = parseCommand(str[i]);
        printf("cmd found = %s from string: %s \n",lgc.cmds[i].raw_command, str[i]);
    }

    lgc.nr_cmds = cnt;
    if(cnt == 0){
        lgc.op[0] = 0;
    }
    return  lgc;
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
    return 0;
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

int executeLogicCommand(LogicCommand lgcmd){
    pid_t slaves[MAX_CHAIN_CMD] ;

    for(int i = 0; i < lgcmd.nr_cmds; i++){
        slaves[i] = fork();
        if(slaves[i] < 0){
            printf("ERROR: Unable to fork\n\n");
            return -1;
        }
        if(slaves[i] == 0){
            executeCommand(lgcmd.cmds[i]);
        }
        else {
            int status = 0;
            wait(&status);
            if(status!=-1)
                return -2;
        }
    }
    return 0;
}

int main() {

    updateCurrentDir();

    while ( 1 ) {
        char* line = readInput();
        (parseLogicCommand(line));
    }


    return 0;
}
#pragma clang diagnostic pop