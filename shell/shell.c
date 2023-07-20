/*在使用strtok()函数分割命令时，需要考虑命令中可能会有多个空格，而不是只考虑单个空格。

在处理重定向时，你需要先判断重定向符号的位置，并且要考虑符号后面是否还有文件名，如果有，则需要将其一起作为参数传递给相应的函数。

在使用管道时，需要注意创建管道的顺序和连接子进程的顺序，以及在最后一个子进程执行完后关闭所有管道和文件描述符。

当子进程运行失败时，需要给出适当的错误提示。*/

/*问题：    后台运行未解决
           前一个cd 了一个错误的目录，然后cd - 不能返回到上一次的目录
           对错误的处理不够完善
           对空格的处理不够完善
           不能实现输入输出重定向同时存在的复杂命令 eg:ls -l < 70.txt | grep tmp
   | wc > 6.txt 信号处理还是有问题，输入一些命令后，ctrl+c就会乱输出
           对美化处理不到位，太丑了


                                                                    */

#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
// 444
void Prompt();
void Pwd();
void ListDirectory();
void ChangeDirectory(char[]);
void ShowEnvironment();
void SetEnvironment(char[]);
void UnSetEnvironment(char[]);
void do_execvp(char**, int, int);
void PipeCommand(char **,int,int);
void signalhandle(int);

volatile sig_atomic_t signal_flag = false;

/*现在只能处理没有管道的命令*/


char home[500];

int main(int argc, char* argv[]) {
    char command[500] = "";
    char backup[500];

    getcwd(home, 500);

    system("clear");

    char* command_son[1024];
    /*在 C 语言中，字符串是以 null
终止的字符数组。这意味着在内存中必须为字符串分配足够的空间来存储每个字符，以及一个用于表示字符串结束的
null 字符。

当您定义一个字符数组时，编译器会为其分配一定的内存空间，但是这个空间的大小是固定的，不能动态调整。因此，如果您想存储不定长度的字符串，或者您不知道要存储的字符串的长度，您需要使用动态内存分配函数，例如
malloc() 函数，来分配足够的内存空间。

在 C
语言中，动态内存分配是一种将内存空间分配给变量的方法，它可以在程序运行时动态地分配和释放内存，从而允许您动态存储和处理数据。在使用动态内存分配时，您需要手动分配和释放内存，以确保内存使用的正确性和效率。*/

    // 注册了一个SIGINT信号[ctrl+c]的处理函数，就是说进程接收到ctrl+c调用signalhandle的信号处理函数处理该信号
    signal(SIGINT, signalhandle);

    // The Main Prompt of Shell
    while (1) {
        // used for printing prompt
        Prompt();
        // char temp[500];

        // getcwd(temp, 500);
        // printf("shell.1.8@ %s \n$ ", temp);
        // used for reading a whole line and removing new line character with
        // null
        if (fgets(command, 500, stdin) == 0)
            exit(0);
        if (command[0] == '\n')
            continue;
        command[strlen(command) - 1] = '\0';

        char command_backup[500];
        strcpy(command_backup, command);

        /*记录pipe数量*/
        int pipe_num = 0;
        for (int i = 0; i < strlen(command); i++) {
            if (command[i] == '|')
                pipe_num++;
        }
        int command_son_num = 0;

        /*如果没有管道符号，就malloc开辟一块内存存放命令*/
        if (pipe_num == 0) {
            command_son[0] =
                (char*)malloc(sizeof(char) * (1 + strlen(command)));
            strcpy(command_son[0], command);
            command_son_num++;

        }
        /*如果有管道符号，将输入的命令按照管道符分为多个子命令，子命令需要根据管道的个数n分别分配n+1块内存*/
        else {
            char* token = strtok(command_backup, "|");

            // while (token != NULL)
            for (int i = 0; token != NULL && i < pipe_num + 1; i++) {
                command_son[i] =
                    (char*)malloc(sizeof(char) * (strlen(token) + 1));
                strcpy(command_son[command_son_num], token);
                command_son_num++;
                token = strtok(NULL, "|");
            }
        }

        // printf("%s\n", command_son[0]);
        // printf("%s\n", command_son[1]);
        // printf("%s\n", command_son[2]);

        /*      command1 < input_file | command2 > output_file
        在这个命令中，<和>分别表示输入重定向和输出重定向。命令执行的顺序如下：

        1.打开文件 input_file 并将其作为 command1 的标准输入。
        2.启动 command1 进程，并将其标准输出连接到管道。
        3.启动 command2 进程，并将其标准输入连接到管道。
        4.打开文件 output_file 并将其作为 command2 的标准输出。
        5.command1 将输出写入管道，command2 从管道读取输入，并将输出写入
        output_file。*/

        /*int dup2(int oldfd, int newfd);
        dup2函数，把指定的newfd也指向oldfd指向的文件*/

        pid_t pid;
        int status;
        pid = fork();
        if (pid < 0) {
            printf("Error : failed to fork\n");
        } /*子进程处理*/
        else if (pid == 0) {
            if (pipe_num != 0)
                PipeCommand(command_son, pipe_num, command_son_num);
            if (command_son[0][0] == 'c' && command_son[0][1] == 'd') {
                ChangeDirectory(command_son[0]);
            }
            if (pipe_num == 0)
                /*如果没有管道，那么command_son里面只有一条命令*/
                do_execvp(command_son, 0, command_son_num);

        } else {
            // printf("awa");
            // pid_t result = wait4(pid, &status, 0, NULL);
            // printf("Child process with PID %d has terminated with status
            // %d\n",
            //        result, status);
            waitpid(pid, &status, 0);
            // fflush(stdin);
            // fflush(stdout);
        }

        // free(command_son[1]);
        // 将输出的颜色改成黄色
        // printf("\033[01;33m");

        /*最终free的地方*/
        for (int i = 0; i < command_son_num; i++) {
            free(command_son[i]);
        }
        fflush(stdin);
        fflush(stdout);
        fflush(stderr);
    }

    exit(0);  // exit normally
}

void Prompt() {
    // for current working directory
    char temp[500];

    getcwd(temp, 500);
    printf("\033[1;32m%s  ", getenv("USER"));
    printf("%s\033[1;31m$ ", temp);
    // }

    return;
}

void Pwd() {
    // for current working directory
    char cwd[500];

    getcwd(cwd, 500);

    printf("%s \n", cwd);

    return;
}

void ListDirectory() {
    struct dirent** listdirectory;

    int ret = scandir(".", &listdirectory, NULL, alphasort);

    if (ret < 0) {
        printf("ERROR SCANNING DIRECTORY\n");
        exit(1);
    }

    else if (ret <= 2) {
        free(listdirectory);
        return;
    }

    else {
        while (ret > 2) {
            ret--;

            // hidden directory
            if (listdirectory[ret]->d_name[0] == '.') {
                free(listdirectory[ret]);
                continue;
            }

            printf("%s  ", listdirectory[ret]->d_name);
            free(listdirectory[ret]);
        }
    }

    free(listdirectory);
    printf("\n");

    return;
}
/*cd*/
char backup_history[500] = {"~"};
char awa[500];
int judge = 0;
char newpath[500];
void ChangeDirectory(char path[]) {
    // cd with no path and & support
    char backup_history_1[500];

    // strcpy(backup_history, newpath);
    // ~

    for (int i = 3; path[i] != '\0'; i++) {
        if (path[i] == '-') {
            if (judge % 2 != 0) {
                memset(backup_history, 0, 500);
                getcwd(backup_history, 500);
                chdir(awa);
            }
            if (judge % 2 == 0) {
                memset(awa, 0, 500);
                getcwd(awa, 500);
                chdir(backup_history);
            }
            judge++;
            return;
        }
    }
    if (chdir(newpath) >= 0) {
        memset(backup_history, 0, 500);
        getcwd(backup_history, 500);
        strcpy(backup_history_1, home);
    }

    // cd with no path
    if (path[0] == 'c' && path[1] == 'd' && path[2] == '\0') {
        if (chdir(home) < 0) /*change directory error*/
        {
            printf("ERROR CHANGING DIRECTORY TO HOME\n");
            exit(1);
        }
    }
    // have path
    else {
        int j = 0;
        for (int i = 3; path[i] != '\0'; i++, j++) {
            if (path[i] == '-') {
                chdir(backup_history);
                return;
            }
            if (path[i] == '~') {
                chdir(backup_history_1);
                return;
            }
            newpath[j] = path[i];
        }

        newpath[j] = '\0';

        if (chdir(newpath) < 0) {
            printf("shell: cd: %s: No such file or directory\n", newpath);
        }
    }
}

void do_execvp(char** command_son, int count, int command_son_num) {
    int symbol = 0;
    // for (int i = 0; i < command_son_num; i++) {
    for (int j = 0; j < strlen(command_son[count]); j++) {
        if (command_son[count][j] == '>' || command_son[count][j] == '<') {
            int fd1, fd2, fd3;
            // int in_fd = STDIN_FILENO, out_fd = STDOUT_FILENO;
            /*如果是输入重定向*/
            if (command_son[count][j] == '<') {
                command_son[count][j] = '\0';
                fd1 = open(command_son[count] + j + 2, O_RDONLY);
                dup2(fd1, STDIN_FILENO);
                symbol = j;
            }
            /*>>*/
            else if (command_son[count][j] == '>' &&
                     command_son[count][j + 1] == '>') {
                command_son[count][j] = '\0';
                command_son[count][j + 1] = '\0';
                fd3 = open(command_son[count] + j + 3,
                           O_WRONLY | O_CREAT | O_APPEND, 0644);
                dup2(fd3, STDOUT_FILENO);
                symbol = j;
            }
            /*如果是输出重定向*/
            else if (command_son[count][j] == '>') {
                // char * file_name =;
                ////使用open函数打开一个文件，该文件将成为新的标准输出
                command_son[count][j] = '\0';
                fd2 = open(command_son[count] + j + 2,
                           O_WRONLY | O_CREAT | O_TRUNC, 0666);
                // 标准输出（stdout）的文件描述符重定向到fd所指向的文件描述符上
                // execvp(command_son[i], command_son + i);
                dup2(fd2, STDOUT_FILENO);
                symbol = j;
            }
            /*非法字符*/
            else {
                perror("error");
            }
        }
    }
    // }
    /*part数组存储command_son的部分*/
    // for(int a=0;a<command_son_num;a++){
    char* part[1024];
    char* token = strtok(command_son[count], " ");
    int part_num = 0;
    int j = 0;
    while (token != NULL) {
        part[j] = token;
        token = strtok(NULL, " ");
        j++;
    }
    part[j] = NULL;
    token = strtok(command_son[count], " ");
    if(execvp(token, part)==-1){
    printf("Cann't found this command\n");}
    exit(1);
    // }
}

void PipeCommand(char** command_son, int pipe_num, int command_son_num) {
    int pipes[pipe_num][2];
    pid_t pid[command_son_num];
    /*创建管道*/
    for (int i = 0; i < pipe_num; i++) {
        if (pipe(pipes[i]) < 0) {
            printf("Pipe falied\n");
            exit(1);
        }
    }
    /*新建pipe_num+1个进程*/
    for (int i = 0; i < command_son_num; i++) {
        if ((pid[i] = fork()) == 0) {
            if (i == 0) {
                // First process
                dup2(pipes[i][1], STDOUT_FILENO);
                /*如果第一个进程不关闭管道的读取端，它会一直等待从管道中读取数据，因为在后续进程中，前面的进程已经将数据通过管道传递给了后面的进程，这时第一个进程已经没有必要从管道中读取数据了。所以第一个进程关闭管道的读取端是为了让后续进程可以从管道中获取数据。*/
                close(pipes[i][0]);
                close(pipes[i][1]);
                // do_execvp(command_son, i, command_son_num);
                // exit(0);
            } else if (i == command_son_num - 1) {
                // Last process
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
                // do_execvp(command_son, i, command_son_num);
                // exit(0);
            } else {
                // Middle process
                dup2(pipes[i - 1][0], STDIN_FILENO);
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
                close(pipes[i][0]);
                close(pipes[i][1]);
                // do_execvp(command_son, i, command_son_num);
                // exit(0);
            }
            for (int i = 0; i < pipe_num; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            do_execvp(command_son, i, command_son_num);
            exit(0);
        }
    }
    for (int i = 0; i < pipe_num; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    /*通过调用waitpid()函数等待子进程的退出，父进程可以及时获得子进程的退出状态，并且在所有子进程都退出之后结束自己的执行，以便能够释放它们占用的系统资源，避免产生僵尸进程。*/
    for (int i = 0; i < command_son_num; i++) {
        waitpid(pid[i], NULL, 0);
    }
}
void signalhandle(int sig) {
    // if(signal_flag)    return;
    // signal_flag =true;
    // printing new line
    printf("\n");

    // printing prompt
    Prompt();

    // emptying buffer
    fflush(stdin);
    fflush(stdout);
    fflush(stderr);
}
