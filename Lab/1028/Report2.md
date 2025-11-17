# Lab2
## Tasks
### 1 Process Priority
#### 1
Implement the `ps` command (`ps.c`).
- The command format should be as follows:
    | PID | PPID | PRI | MEM | STATE | CMD |
    | - | - | - | - | - | - |
    | 1 | N/A | 10 | 12288 | SLEEPING | init |
    | 2 | 1 | 10 | 16384 | SLEEPING | sh |
    | 4 | 2 | 10 | 12288 | RUNNING | ps |
- Implement the `getptable()` system call in `proc.c`
- Call `getptable()` in the `ps` command to display the process list

#### 2
Implement a user-space program (`nice.c`) to modify process priority at runtime. The nice command should be invoked as follows:
- `nice pid priority` (priority values are in the range [1, 20], where a smaller value indicates higher priority)
- Implement the system call (`setpriority(pid, priority)`)

### 2 Semaphore Implementation
Implement the following functions (`proc.c`):
- `int sem_init(int sem, int value)`
    Initializes the value of the semaphore structure
    `int sem`: This parameter is the semaphore number
    `int value`: This parameter is the value of the semaphore
    Returns 0 if initialization is successful, otherwise returns -1
- `int sem_destroy(int sem)`
    Destroys the semaphore structure
    `int sem`: This parameter is the semaphore number
    Returns 0
- `int sem_wait(int sem, int count)`
    Allows a process/thread to acquire exclusive access to a shared resource using a semaphore, or to wait for another process to release that resource
    `int sem`: This parameter is the semaphore number
    `int count`: This parameter is a value used for comparison with the semaphore's value
    Returns 0
- `int sem_signal(int sem, int count)`
    Allows a process to release a resource exclusively held via a semaphore
    `int sem`: This parameter is the semaphore number
    `int count`: This parameter is a value used for comparison with the semaphore's value
    Returns 0

### 3 Solve File Read/Write Mutual Exclusion Using Semaphores
Create the file `sem_test.c` and implement the following process:
- Create 10 processes, each reading a numerical value from a file and incrementing it by 1. This process should be repeated 50 times
- Ensure that after mutual exclusion, the final value written to the file is the sum of all process counts, 500

## Implementation
### Environment
- M4 MacBook Air
- VMware Fusion Professional Version 13.6.3 (24585314)
- `Linux ubuntuserver 6.14.0-35-generic #35~24.04.1-Ubuntu SMP PREEMPT_DYNAMIC Tue Oct 14 13:30:46 UTC 2 aarch64 aarch64 aarch64 GNU/Linux`

### Features

### Code
#### 1
##### 1
- `proc.c`
    ```c
    //...

    int 
    getptable(int nproc, int size, char *buffer){
    /*
    */
    if(buffer)
    {
        struct proc *p_src;
        struct proc_us *p_dst=(struct proc_us*)buffer;
        acquire(&(ptable.lock));
        p_src=ptable.proc;
        for(int i=0;(i<nproc)&&(((char *)(p_dst+1)-buffer)<size)&&(p_src-(ptable.proc))<NPROC;++p_src)
        if(p_src->state==UNUSED)
            continue;
        else
            p_dst->sz=(p_src->sz),p_dst->state=(p_src->state),p_dst->pid=(p_src->pid),p_dst->ppid=(p_src->parent)?(p_src->parent->pid):-1,p_dst->priority=(p_src->priority),memmove(p_dst->name,p_src->name,sizeof(p_src->name)),p_dst->ctime=(p_src->ctime),p_dst->stime=(p_src->stime),p_dst->retime=(p_src->retime),p_dst->rutime=(p_src->rutime),++p_dst,++i;
        release(&(ptable.lock));
        return 0;
    }
    return -1;
    }

    ```
- `ps.c`
    ```c
    //...

    int main(int argc, char *argv[])
    {
        /*
        */
        struct proc_us ptable[NPROC] = {0};
        if (getptable(NPROC, NPROC * sizeof(struct proc_us), &ptable) >= 0)
        {
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
        printf(2, "ps: error getting ptable");
        exit();
    }

    ```

##### 2
- `proc.c`
    ```c
    int
    setpriority(int pid, int priority)
    {
    /*
    */
    if((pid>=0)&&(priority>=0))
    {
        short flag=0;
        acquire(&(ptable.lock));
        for(struct proc *p=ptable.proc;(p-(ptable.proc)<NPROC);++p)
        if((p->pid==pid)&&(p->state!=UNUSED))
        {
            p->priority=priority,flag=1;
            break;
        }
        release(&(ptable.lock));
        return flag?pid:-1;
    }
    return -1;
    }

    ```
- `nice.c`
    ```c
    //...

    #define MIN_PRIO 1  //
    #define MAX_PRIO 20 //

    int main(int argc, char *argv[])
    {
        /*
        */
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

    ```

#### 2
- `int sem_init(int sem, int value)`
    ```c
    int sem_init(int sem, int value)
    {
    /*
    */
    if((sem>=0)&&(sem<MAX_SEMA_NUM))
    {
        acquire(&(sema[sem].lock));
        if(!(sema[sem].active))
        {
        sema[sem].active=1;
        sema[sem].value=value;
        }
        release(&(sema[sem].lock));
        return 0;
    }
    return -1;
    }

    ```
- `int sem_destroy(int sem)`
    ```c
    int
    sem_destroy(int sem)
    {
    /*
    */
    if((sem>=0)&&(sem<MAX_SEMA_NUM))
    {
        acquire(&(sema[sem].lock));
        sema[sem].active=0;
        release(&(sema[sem].lock));
        return 0; 
    }
    return -1;
    }

    ```
- `int sem_wait(int sem, int count)`
    ```c
    int sem_wait(int sem, int count)
    {
    /*
    */
    if((sem>=0)&&(sem<MAX_SEMA_NUM))
    {
        acquire(&(sema[sem].lock));
        if(sema[sem].value>=count)
        sema[sem].value-=count;
        else
        {
        while(sema[sem].value<count)
            sleep(sema+sem,&(sema[sem].lock));
        sema[sem].value-=count;
        }
        release(&sema[sem].lock);
        return 0;
    }
    return -1;
    }

    ```
- `int sem_signal(int sem, int count)`
    ```c
    int sem_signal(int sem, int count)
    {
    /*
    */
    if((sem>=0)&&(sem<MAX_SEMA_NUM))
    {
        acquire(&(sema[sem].lock));
        sema[sem].value+=count;
        wakeup(&sema[sem]);
        release(&sema[sem].lock);
        return 0;
    }
    return -1;
    }

    ```

#### 3
- `sem_test.c`
    ```c
    //...

    #define MAX_NUM_LEN 3 //
    #define SEMA_IDX 0 //

    /*
    */
    int file_init(char *f_name, int init_val)
    {
        int fd = open(f_name, O_CREATE | O_WRONLY);
        if (fd >= 0)
        {
            printf(fd, "%d\n", init_val);
            close(fd);
            return 0;
        }
        printf(2, "sem_test: file_init: failed to initialize file (%s)\n", f_name);
        return -1;
    }

    /*
    */
    int get_file_cnt(char *f_name)
    {
        int fd = open(f_name, O_RDONLY), val = -1;
        if (fd >= 0)
        {
            char buf[MAX_NUM_LEN + 1];
            int n = read(fd, buf, MAX_NUM_LEN);
            if (n > 0)
            {
                buf[n] = 0;
                val = atoi(buf);
            }
            close(fd);
        }
        if (val < 0)
            printf(2, "sem_test: get_file_cnt: failed to read file (%s)\n", f_name);
        return val;
    }

    /*
    */
    int set_file_cnt(char *f_name, int val)
    {
        unlink(f_name);
        int fd = open(f_name, O_CREATE | O_WRONLY);
        if (fd >= 0)
        {
            printf(fd, "%d\n", val);
            close(fd);
            return 0;
        }
        printf(2, "sem_test: set_file_cnt: failed to write file (%s)\n", f_name);
        return -1;
    }

    /*
    */
    void child_proc()
    {
        printf(1, "sem_test: child started\n");
        sleep(NUM_CHILDREN);
        short flag = 1;
        for (int i = 0; flag && (i < TARGET_COUNT_PER_CHILD); ++i)
            if (sem_wait(SEMA_IDX, 1))
                flag = 0;
            else
            {
                int val = get_file_cnt(COUNTER_FILE);
                if (val < 0)
                    flag = 0;
                else
                {
                    ++val;
                    if (set_file_cnt(COUNTER_FILE, val))
                        flag = 0;
                }
                if (sem_signal(SEMA_IDX, 1))
                    flag = 0;
            }
        if (!flag)
            printf(2, "sem_test: child_proc: error occurred in child process\n");
        exit();
    }

    int main(int argc, char **argv)
    {
        /*
        */
        short flag = 1;
        if (!sem_init(SEMA_IDX, 1))
        {
            if (!file_init(COUNTER_FILE, 0))
            {
                for (int i = 0; flag && (i < NUM_CHILDREN); ++i)
                {
                    int pid = fork();
                    if (pid < 0)
                    {
                        flag = 0;
                        printf(2, "sem_test: main: fork failed\n");
                        for (; i; --i)
                            wait();
                    }
                    else if (pid == 0)
                        child_proc();
                }
                for (int i = 0; flag && (i < NUM_CHILDREN); ++i)
                    if (wait() < 0)
                        flag = 0, printf(2, "sem_test: main: wait failed\n");
                if (flag)
                {
                    int res = get_file_cnt(COUNTER_FILE), expt = NUM_CHILDREN * TARGET_COUNT_PER_CHILD;
                    if (res < 0)
                        flag = 0;
                    else if (res != expt)
                        printf(1, "sem_test: test failed (cnt=%d, expt=%d)\n", res, expt);
                    else
                        printf(1, "sem_test: test passed (cnt=%d)\n", res);
                }
                flag = !flag;
            }
            if (sem_destroy(SEMA_IDX))
                flag = 1;
        }
        if (flag)
            printf(2, "sem_test: test internal error\n");
        exit();
    }

    ```

## Results
