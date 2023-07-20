#include <linux/limits.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>

// 颜色参数
#define WHITE 0
#define BLUE 1
#define GREEN 2
#define RED 3
#define LBLUE 4
#define YELLOW 5

// 命令行参数
#define PARAM_NONE 0
//-a 显示所有文件
#define PARAM_A 1
//-l 一行只显示一个文件的详细信息
#define PARAM_L 2
//-R 递归打开目录显示文件
#define PARAM_R 4
//-r 倒序显示文件
#define PARAM_r 8
//-t 按照时间排序
#define PARAM_t 16
//-i 查看文件的inode号
#define PARAM_i 32
//-s 显示文件大小
#define PARAM_s 64
//-F 识别文件类型 ，加上标识符[暂时未写]
#define PARAM_F 128


// 设置每行最多的字符数为80
#define MAXROWLEN 80

char str[12] = "-----------";
int g_leave_len = MAXROWLEN; // 一行剩余长度，用于输出对齐
int g_maxlen;                // 存放某目录下最长文件名的长度
int num = 0;
char filenames[4096*16][PATH_MAX + 1], temp[PATH_MAX + 1];
// 带有不同颜色的打印文件名
void printf_name(char *name, int color); 
// 不同文件类型得到不同颜色型号
int get_color(struct stat buf);          
// 为显示某个目录下的文件做准备
void display_dir(int flag_param, char *path);
// 递归打开目录
void display_dir_R(int flag, char *name);
// 打印名为name的文件信息
void display_attribute(struct stat buf, char *name, int, int);
// 输出文件的文件名
void display_single(int flag, struct stat buf, char *name, int color);
// 根据命令行参数flag和完整路径名，显示目标文件
void display(int flag, char *pathname);
void display_dir_t(char filenames[4096*16][PATH_MAX+1],int count);


int main(int argc, char *argv[])
{
    int i, j, k;
    char path[PATH_MAX + 1];
    memset(path, 0, PATH_MAX + 1);
    char param[32];              // 保存命令行参数，目标文件名和目录名不在此列
    int flag_param = PARAM_NONE; // 参数种类，默认为0
    struct stat buf;
    j = 0;

    /*--------------------------参数的数字化------------------------------------------*/
    // 对命令行参数进行解析，提取到param数组中
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            for (k = 1; k < strlen(argv[i]); k++, j++)
            {
                param[j] = argv[i][k]; // 获取-后面的参数保存到数组param中
            }
            num++; // 保存 - 的个数
        }
    }

    // 让flag_param 得到总参数，并对错误命令行参数进行筛选
    for (i = 0; i < j; i++)
    {
        if (param[i] == 'a')
        {
            flag_param |= PARAM_A;
            continue;
        }
        else if (param[i] == 'l')
        {
            flag_param |= PARAM_L;
            continue;
        }
        else if (param[i] == 'R')
        {
            flag_param |= PARAM_R;
        }
        else if (param[i] == 'r')
        {
            flag_param |= PARAM_r;
        }
        else if (param[i] == 't')
        {
            flag_param |= PARAM_t;
        }
        else if (param[i] == 'i')
        {
            flag_param |= PARAM_i;
        }
        else if (param[i] == 's')
        {
            flag_param |= PARAM_s;
        }
        else
        {
            printf("my_ls :invalid option -%c \n", param[i]);
            exit(1);
        }
    }
    param[j] = '\0';
    /*------------------------------------------------------------------------------*/
    // 如果没有输入路径，就设置为当前路径
    if ((num + 1) == argc)
    {
        strcpy(path, "./");
        path[2] = '\0';
        printf("%s:\n",path);
        if(!(flag_param &PARAM_R))
        display_dir(flag_param, path);
        else{
            display_dir_R(flag_param, path);

        }
        return 0;
    }
    i = 1;
    do
    {
        // 如果不是目标文件名或目录，解析下一个命令行参数
        if (argv[i][0] == '-')
        {
            i++;
            continue;
        }
        else
        {
            strcpy(path, argv[i]);

            // 如果目标文件或目录不存在，报错并退出程序
            if (stat(path, &buf) == -1){
                perror("stat fail:");
                exit(1);
            }
            // 判断该路径是不是目录
            if (S_ISDIR(buf.st_mode))
            {
                // 是目录但是没带 '/'就加上'/'
                if (path[strlen(argv[i]) - 1] != '/')
                {
                    path[strlen(argv[i])] = '/';
                    path[strlen(argv[i]) + 1] = '\0';
                }
                // 如果有-R就递归
                if (flag_param & PARAM_R)
                {
                    display_dir_R(flag_param, path);
                }
                else // argv[i]是一个文件
                    printf("%s:\n",path);
                    display_dir(flag_param, path);

                i++;
            }
            else
            {
                display(flag_param, path);
                i++;
            }
        }
    } while (i < argc);
    return 0;
}

void display_attribute(struct stat buf, char *name, int flag, int color)
{
    char buf_time[32];
    struct passwd *psd; // 从该结构体中获取文件所有者的用户名
    struct group *grp;  // 从该结构体中获取文件所有者所属组的组名
    if (flag & PARAM_i || flag & PARAM_s)
    {
        if (flag & PARAM_i)
            printf("%-8d", buf.st_ino);
        if (flag & PARAM_s)
            printf("%-8d", buf.st_blocks / 2);
    }

    // // 获取并打印文件类型
    int mode = buf.st_mode;
    if (S_ISDIR(mode))
        str[0] = 'd'; 
    if (S_ISCHR(mode))
        str[0] = 'c'; 
    if (S_ISBLK(mode))
        str[0] = 'b'; 
    if (S_ISLNK(mode))
        str[0] = 'l'; // 链接文件
    if (S_ISFIFO(mode))
        str[0] = 'p'; //*FIFO又称作命名管道，可用于任意两个进程之间的通信。
    if (S_ISSOCK(mode))
        str[0] = 's'; //*套接字

    // 3 bits for user
    if (mode & S_IRUSR)
        str[1] = 'r';
    if (mode & S_IWUSR)
        str[2] = 'w';
    if (mode & S_IXUSR)
        str[3] = 'x';

    // 3 bits for group
    if (mode & S_IRGRP)
        str[4] = 'r';
    if (mode & S_IWGRP)
        str[5] = 'w';
    if (mode & S_IXGRP)
        str[6] = 'x';

    // 3 bits for other
    if (mode & S_IROTH)
        str[7] = 'r';
    if (mode & S_IWOTH)
        str[8] = 'w';
    if (mode & S_IXOTH)
        str[9] = 'x';
    printf("%s ",str);

    // 根据uid与gid获取文件所有者的用户名与组名
    psd = getpwuid(buf.st_uid);
    grp = getgrgid(buf.st_gid);
    printf("%4d", buf.st_nlink); // 打印文件的链接数
    printf("%-8s", psd->pw_name);
    printf("%-8s", grp->gr_name);
    printf("%6d", buf.st_size); // 打印文件的大小
    strcpy(buf_time, ctime(&buf.st_mtime));
    buf_time[strlen(buf_time) - 1] = '\0'; // 去掉换行符
    printf(" %s", buf_time);               // 打印文件的时间信息
    printf(" ");
    printf_name(name, color);
    printf("\n");
}

void display_single(int flag, struct stat buf, char *name, int color)
{
    int i, len;

    // 判断是否带有i,s参数
    if (flag & PARAM_i || flag & PARAM_s)
    {
        if (flag & PARAM_i)
            printf("%-8d", buf.st_ino);
        if (flag & PARAM_s)
            printf("%-8d", buf.st_blocks / 2);

        printf(" ");
        printf_name(name, color);
        printf("\n");
    }

    // 判断是否带有s参数
    //  if(flag & PARAM_s)
    //  {

    //     printf_name(name, color);
    //     printf("\n");
    // }

    else
    {
        /*判断最长的文件名能否在剩余的位置放下*/
        if (g_leave_len < g_maxlen) /*放不下就下一行*/
        {
            printf("\n");
            g_leave_len = MAXROWLEN; /*转到下一行，下一行的长度初始化*/
        }
        len = g_maxlen - strlen(name); /*剩余长度*/
        printf_name(name, color);
        }
        for (i = 0; i < len; i++)
        {
            printf(" ");
        }
        printf("  ");
        g_leave_len -= (g_maxlen + 3);
    }

// 根据命令行参数和完整路径名显示目标文件
// 参数flag：命令行参数
// 参数pathname：包含了文件名的路径名
//
void display(int flag, char *pathname)
{
    int i, j;
    struct stat buf;
    char name[NAME_MAX + 1];

    // 过滤掉路径，仅保留文件名到name
    // 从路径中解析出文件名
    for (i = 0, j = 0; i < strlen(pathname); i++)
    {
        if (pathname[i] == '/')
        {
            j = 0;
            continue;
        }
        name[j++] = pathname[i];
    }
    name[j] = '\0';
    // 用lstat而不是stat以方便解析链接文件
    if (lstat(pathname, &buf) == -1)
    {
            perror("opendir ");
            // exit(1);
            return;
        
    }
    int color = get_color(buf);
    // display(int flag, char *pathname)
    // display(flag_param, filenames[i]);
    // for(int k=0;k<num;k++){
    if(flag & PARAM_A)
    {
        if (flag & PARAM_L)
        {
            display_attribute(buf, name, flag, color);
        }
        else
        {
            display_single(flag, buf, name, color);
        }
    }
    else if (name[0] != '.')
    {
        if (flag & PARAM_L)
        {
            display_attribute(buf, name, flag, color);
        }
        else
        {
            display_single(flag, buf, name, color);
        }
    }
}
void display_dir(int flag_param, char *path)
{

    DIR *dir;
    g_leave_len = 80;
    struct dirent *ptr;
    struct stat tem;
    int count = 0;
    // char filenames[2000][PATH_MAX + 1], temp[PATH_MAX + 1];
    // 获取该目录下文件总数和最长的文件名
    dir = opendir(path);
    if (dir == NULL)
    {
            perror("fail:");
            // exit(1);
            return;
    }
    
    
    // 找到最长文件名，并且统计文件个数
    while ((ptr = readdir(dir)) != NULL)
    {
        if (g_maxlen < strlen(ptr->d_name))
            g_maxlen = strlen(ptr->d_name);
        count++;
    }
    if (count > 4096*16)
    {
        printf("too many files under this dir\n");
        exit(0);
    }
    closedir(dir);

    int i, j;
    dir = opendir(path);
    if (dir == NULL)
        perror("opendir fail:");
    int len = strlen(path); // 存储文件名长度
    // 获取该目录下所有的文件名
    for (i = 0; i < count; i++)
    {
        ptr = readdir(dir);
        if (ptr == NULL)
            perror("readdir fail:");
        strncpy(filenames[i], path, len);  // 将文件名传递到 filenames中存储
        filenames[i][len] = '\0';          // 文件名后加'\0'表示结束
        strcat(filenames[i], ptr->d_name); //
        filenames[i][len + strlen(ptr->d_name)] = '\0';
    }

    // 冒泡法排序，排序后文件名按字母顺序存储于filenames
    for (i = 0; i < count - 1; i++)
    {
        for (j = 0; j < count - 1 - i; j++)
        {
            if (strcmp(filenames[j], filenames[j + 1]) > 0)
            {
                strcpy(temp, filenames[j + 1]);
                temp[strlen(filenames[j + 1])] = '\0';

                strcpy(filenames[j + 1], filenames[j]);
                filenames[j + 1][strlen(filenames[j])] = '\0';

                strcpy(filenames[j], temp);
                filenames[j][strlen(temp)] = '\0';
            }
        }
    }
    // char *myls_t(struct stat **temp, int num,char *filenames[])
    // myls_t(tem,count,filenames);
    if(PARAM_t & flag_param )
    display_dir_t(filenames, count);
    if (flag_param & PARAM_r)
    {
        for (i = count - 1; i >= 0; i--)
        {
            display(flag_param, filenames[i]);
        }
    }
    else // 不带参数-r，正序输出
        for (i = 0; i < count; i++)
        {
            // if (filenames[i][0] != '.')
            //     continue;
            display(flag_param, filenames[i]);
        }

    closedir(dir);

    // 如果命令行中没有-l选项，打印一个换行符
    if ((flag_param & PARAM_L) == 0)
        printf("\n");
}

void display_dir_R(int flag, char *name)
{
    DIR *dir;
    struct dirent *ptr;
    char temp_name[PATH_MAX + 1];
    printf("%s:\n", name);

    display_dir(flag, name);
    printf("\n");

    if ((dir = opendir(name)) == NULL)
    {
        if (errno != 13)
        {
            perror("display_R fail:");
            return;
        }

        else
            return;
    }
    int len = strlen(name);
    while ((ptr = readdir(dir)) != NULL)
    {
        if (ptr->d_type == DT_DIR && (ptr->d_name[0] != '.'))
        {
            strncpy(temp_name, name, len);
            temp_name[len] = '\0';

            strcat(temp_name, ptr->d_name);
            temp_name[len + strlen(ptr->d_name)] = '/';

            temp_name[len + strlen(ptr->d_name) + 1] = '\0';
            display_dir_R(flag, temp_name);
        }
    }
    closedir(dir);
    return;
}

// 对不同的文件类型给不同的颜色
int get_color(struct stat buf)
{
    int color = 0;
    if (S_ISLNK(buf.st_mode))
    {
        color = LBLUE;
    }
    else if (S_ISDIR(buf.st_mode))
    {
        color = BLUE;
    }
    else if (S_ISCHR(buf.st_mode) || S_ISBLK(buf.st_mode))
    {
        color = YELLOW;
    }
    else if (buf.st_mode & S_IXUSR)
    {
        color = GREEN;
    }
    return color;
}

void printf_name(char *name, int color)
{
    if (color == GREEN)
    {
        printf("\033[1m\033[32m%-s\033[0m", name);
    }
    else if (color == BLUE)
    {
        printf("\033[1m\033[34m%-s\033[0m", name);
    }
    else if (color == WHITE)
    {
        printf("%-s", name);
    }
    else if (color == LBLUE)
    {
        printf("\033[1m\033[36m%-s\033[0m", name);
    }
    else if (color == YELLOW)
    {
        printf("\033[1m\033[33m%-s\033[0m", name);
    }
}



void display_dir_t(char filenames[4096*16][PATH_MAX + 1],int count) {
    char temp[2048] = {0};
    struct stat info1;
    struct stat info2;
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            stat(filenames[i], &info1);
            stat(filenames[j], &info2);
            if (info1.st_mtime < info2.st_mtime) {
                strcpy(temp, filenames[i]);
                strcpy(filenames[i], filenames[j]);
                strcpy(filenames[j], temp);
            }
        }
    }
}