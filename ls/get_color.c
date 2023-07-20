// 颜色参数
#define WHITE 0
#define BLUE 1
#define GREEN 2
#define RED 3
#define LBLUE 4
#define YELLOW 5



//去判断文件的类型，然后去返回一个常数
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
