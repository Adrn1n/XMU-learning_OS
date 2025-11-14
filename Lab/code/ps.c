#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"

int main(int argc, char *argv[])
{
    struct proc_us ptable[NPROC] = {0};
    if (getptable(NPROC, NPROC * sizeof(struct proc_us), &ptable) < 0)
    {
        printf(2, "ps: error getting ptable");
        exit();
    }
    printf(1, "PID\tPPID\tPRI\tMEM\tSTATE   \tCMD\n");
    for (struct proc_us *p = ptable; (p - ptable) < NPROC; ++p)
        if (p->state != UNUSED)
        {
            printf(1, "%d\t", p->pid);
            if (p->ppid >= 0)
                printf(1, "%d\t", p->ppid);
            else
                printf(1, "N/A\t");
            printf(1, "%d\t%d\t", p->priority, p->sz);
            switch (p->state)
            {
            case EMBRYO:
                printf(1, "EMBRYO  \t");
                break;
            case SLEEPING:
                printf(1, "SLEEPING\t");
                break;
            case RUNNABLE:
                printf(1, "RUNNABLE\t");
                break;
            case RUNNING:
                printf(1, "RUNNING \t");
                break;
            case ZOMBIE:
                printf(1, "ZOMBIE  \t");
                break;
            default:
                printf(2, "ERROR   \t");
            }
            printf(1, "%s\n", p->name);
        }
    exit();
}
