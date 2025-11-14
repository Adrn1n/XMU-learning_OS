#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define MIN_PRIO 1
#define MAX_PRIO 20

int main(int argc, char *argv[])
{
    if (argc == 3)
    {
        int pid = atoi(argv[1]), prio = atoi(argv[2]);
        if (pid >= 0)
            if ((prio >= MIN_PRIO) && (prio <= MAX_PRIO))
            {
                if (setpriority(pid, prio) != pid)
                    printf(2, "nice: error to set pid=%d with prio=%d\n", pid, prio);
            }
            else
                printf(2, "nice: invalid param(prio=%d), should be in [%d,%d]\n", prio, MIN_PRIO, MAX_PRIO);
        else
            printf(2, "nice: invalid param(pid=%d)\n", pid);
    }
    else
        printf(2, "nice: usage: nice <pid> <prio>\n");
    exit();
}
