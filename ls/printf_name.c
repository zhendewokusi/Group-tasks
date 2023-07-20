
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
