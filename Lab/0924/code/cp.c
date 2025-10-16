#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define BUF_SIZE 512

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf(2, "cp: Usage: cp <src> <dst>\n");
        exit();
    }
    if (strcmp(argv[1], argv[2]) == 0)
    {
        printf(2, "cp: src(%s) and dst(%s) are the same\n", argv[1], argv[2]);
        exit();
    }
    int rfd = open(argv[1], O_RDONLY);
    if (rfd < 0)
    {
        printf(2, "cp: failed to open %s\n", argv[1]);
        exit();
    }
    uint len = strlen(argv[2]);
    if (argv[2][len - 1] == '/')
    {
        char *pos = argv[1], *tmp = pos;
        while ((tmp = strchr(pos, '/')) && (*++tmp))
            pos = tmp;
        strcpy(argv[2] + len, pos);
    }
    unlink(argv[2]);
    int wfd = open(argv[2], O_CREATE | O_WRONLY);
    if (wfd < 0)
    {
        printf(2, "cp: failed to create %s\n", argv[2]);
        close(rfd);
        exit();
    }
    char buf[BUF_SIZE] = {0};
    int n = 0;
    while ((n = read(rfd, buf, sizeof(buf))) > 0)
        if (write(wfd, buf, n) != n)
        {
            printf(2, "cp: write error to %s\n", argv[2]);
            break;
        }
    if (n < 0)
        printf(2, "cp: read error on %s\n", argv[1]);
    close(rfd);
    close(wfd);
    exit();
}
