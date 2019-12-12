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
#define NUM_COMMANDS (3)
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
}PipeCommand;

typedef struct{
    PipeCommand pcmds[MAX_CHAIN_CMD];
    int nr_pcmds;
    int op[MAX_CHAIN_CMD - 1];
}LogicCommand;



char *currentDir[BUFF_SIZE];

char *custom_commands[NUM_COMMANDS] = {
        "cd",
        "exit",
        "help"
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
    updateCurrentDir();
    sprintf(prompt, "%s:%s: ", getUserName(),  currentDir);
    char* line = readline(prompt);
    return line;
}


Command parseCommand(char *line) {

    int nr_args = 0;
    char *str[MAX_NR_ARGS];
    char *ptr;
    char sep[] = " "; /// split by space
    ptr = strtok(line,sep);
    while(ptr != NULL){
        str[nr_args] = malloc(strlen(ptr) + 1);
        strcpy(str[nr_args], ptr);

        ptr = strtok(NULL,sep);
        nr_args++;
    }

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

PipeCommand parsePipeCommand(const char *line) {

    int cnt = 0;
    char *str[MAX_CHAIN_CMD];
    PipeCommand pcmd;
    str[0] = malloc(BUFF_SIZE);

    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == '|') {
            cnt++;
            str[cnt] = malloc(BUFF_SIZE);
            i++;
        }
    }

    cnt = 0;

    for (int i = 0, idx = 0; line[i] != '\0'; i++) {
        if (line[i] == '|') {
            cnt++;
            i++;
            idx = 0;
        } else {
            str[cnt][idx] = line[i];
            str[cnt][idx + 1] = '\0';
            idx++;
        }
    }

    for (int i = 0; i <= cnt; i++) {

        pcmd.cmds[i] = parseCommand(str[i]);
        free(str[i]);
    }
    pcmd.nr_cmds = cnt + 1;


    return pcmd;
}

LogicCommand parseLogicCommand(const char *line){
    int cnt = 0;
    char *str[MAX_CHAIN_CMD];
    LogicCommand lgc;

    str[0] = malloc(BUFF_SIZE);
    for(int i = 0; line[i] != '\0'; i++){
        if( (line[i] == '&' && line[i + 1] == '&') || (line[i] == '|' && line[i + 1] == '|')){
            cnt++;
            str[cnt] = malloc(BUFF_SIZE);
            i++;
        }
    }

    cnt = 0;
    for(int i = 0, idx = 0; line[i] != '\0'; i++){
        if( (line[i] == '&' && line[i + 1] == '&') || (line[i] == '|' && line[i + 1] == '|')) {
            cnt++;
            i++;
            lgc.op[cnt - 1] = 0;
            if (line[i] == '|' ) lgc.op[cnt - 1] = 1;
            idx = 0;
        } else{
            str[cnt][idx] = line[i];
            str[cnt][idx+1] = '\0';
            idx++;
        }
    }


    for(int i = 0;  i <= cnt; i++){
        lgc.pcmds[i] = parsePipeCommand(str[i]);
        free(str[i]);
    }

    lgc.nr_pcmds = cnt;
    if(cnt == 0){
        lgc.op[0] = 0;
    }
    return  lgc;
}

void openHelp(){
    puts("\n  Welcome! I heard that you need help. See documentation :)");
}

/**
 * only call this from a child process
 * @param currCommand
 * @return
 */
int executeCustomCommand(Command currCommand){
    switch (currCommand.id_custom_command){
        case 0: {
            char *directory=malloc(BUFF_SIZE);

            getcwd(directory,BUFF_SIZE);
            printf("dir before: %s\n",directory);


            chdir(currCommand.arguments[1]);

            getcwd(directory, sizeof(directory));
            printf("dir after: %s\n",directory);
            return 0;
            break;
        }
        case 1:{
            printf("GOODBYE\n");
            exit(0);
        }
        case 2:{
            openHelp();
            return 1;
        }
        default: {
            printf("Unknown command\n");
            return -1;
        }
    }
}

int executeSystemCommand(Command cmd) {

    pid_t worker = fork();
    if (worker < 0) {
        printf("Error: Unable to fork child \n\n");
        return -1;
    }
    if (worker == 0) {
        execvp(cmd.raw_command, cmd.arguments );
//        int wtf = 1 / 0;
        exit(SIGILL);
    } else {
        int status = 0;
        wait(&status);
        return status;
    }
    return 0;
}
/**
 * only call this from a process child
 * @param currCommand
 * @return
 */
int executeCommand(Command currCommand){
    if(currCommand.commandType == CUSTOM){
        return ( executeCustomCommand(currCommand));

    }
    if (currCommand.commandType == SYSTEM) {
        exit( executeSystemCommand(currCommand));
    }

    return -1;
}

int spawnProcess(int in, int out, Command cmd) {
    pid_t pid;
    if ( (pid = fork() == 0) ) {
        if (in != STDIN_FILENO) {
            dup2(in, STDIN_FILENO);
            close(in);
        }

        if (out != STDOUT_FILENO) {
            dup2(out, STDOUT_FILENO);
            close(out);
        }

        return executeCommand(cmd);
    } else {
        wait(NULL);
    }
    return pid;
}

int executePipeCommand2(Command cmd1, Command cmd2) {
    int pid;
    int pipefd[2];

    pipe(pipefd);

    pid = fork();
    if (pid == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]);
        executeCommand(cmd2);
    } else {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);

        executeCommand(cmd1);

        wait(NULL);
    }


    return 0;
}

int executePipeCommand(PipeCommand pcmd) {

    if(pcmd.nr_cmds == 1){
        executeCommand(pcmd.cmds[0]);
        return 0;
    }

    int in, pfd[2];

    in = STDIN_FILENO;
    for (int i = 0; i < pcmd.nr_cmds - 1; i++) {
        pipe(pfd);

        spawnProcess(in, pfd[1], pcmd.cmds[i]);

        close(pfd[1]);

        in = pfd[0];
    }

    if (in != STDIN_FILENO) {
        dup2(in, STDIN_FILENO);
    }

    return executeCommand(pcmd.cmds[pcmd.nr_cmds - 1]);

}

int executeLogicCommand(LogicCommand lgcmd){
    pid_t slaves[MAX_CHAIN_CMD] ;

    for(int i = 0; i <= lgcmd.nr_pcmds; i++){


        /// cd must run on parent process
        if (lgcmd.pcmds[i].nr_cmds == 1) {
            if (lgcmd.pcmds[i].cmds[0].commandType == CUSTOM) {
                executeCommand(lgcmd.pcmds[i].cmds[0]);
                continue;
            }
        }

        slaves[i] = fork();
        if(slaves[i] < 0){
            printf("ERROR: Unable to fork\n\n");
            return -1;
        }

        if(slaves[i] == 0){
            executePipeCommand(lgcmd.pcmds[i]);
        } else {
            int status = 0;
            waitpid(-1, &status, 0);
            if (status != 0)
                printf("command not found\n" );
            if(status != 0 && lgcmd.op[i] == 0)
                return 0;
            if (status == 0 && lgcmd.op[i] == 1) {
                return 0;
            }
        }
    }
    return 0;
}



int main() {

    updateCurrentDir();

    while ( 1 ) {
        char* line = readInput();
        if(strlen(line) > 0){
            add_history(line);
            executeLogicCommand(parseLogicCommand(line));
        }

    }


    return 0;
}
#pragma clang diagnostic pop