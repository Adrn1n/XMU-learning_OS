# Lab2
## Tasks
### 1. Process Priority
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

### 2. Semaphore Implementation
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

### 3. Solve File Read/Write Mutual Exclusion Using Semaphores
Create the file `sem_test.c` and implement the following process:
- Create 10 processes, each reading a numerical value from a file and incrementing it by 1. This process should be repeated 50 times
- Ensure that after mutual exclusion, the final value written to the file is the sum of all process counts, 500

## Environment
- M4 MacBook Air
- VMware Fusion Professional Version 13.6.3 (24585314)
- `Linux ubuntuserver 6.14.0-35-generic #35~24.04.1-Ubuntu SMP PREEMPT_DYNAMIC Tue Oct 14 13:30:46 UTC 2 aarch64 aarch64 aarch64 GNU/Linux`

## Implementation
### Features
#### 1. Process Priority
##### 1
- `getptable()`
    - Can only hold information of up to `nproc` and not exceed the `size` of the buffer
    - Skips processes in the `UNUSED` state
    - Holds `ptable`, so any other process which tries to access it will be blocked until the lock is released by the current process
- `ps.c`
    - Prints like the format specified in the task description
    - Skips processes in the `UNUSED` state
    - Uses a single `\t` to align columns, but when the length difference before the tab is large enough to reach different tab stops, each row aligns differently, causing misaligned columns
    - `STATE` column is aligned by adding spaces after state strings, making it the same length (aligned to the max length of state strings)

##### 2
- `nice.c`
    - Since `atoi()` can't handle negative numbers, so the behavior may be unexpected when negative numbers are input
- `setpriority()`
    - Unlimited priority values, but negative values are not allowed
    - Holds `ptable`, so any other process which tries to access it will be blocked until the lock is released by the current process
- `proc.c`
    - Since the defult scheduler does not consider priority, and all other schedulers are not implemented, changing priority has no effect on process scheduling but only changes the stored priority value

#### 2. Semaphore Implementation
- `sem_init()`
    - Returns -1 when error occurs, but doesn't view initializing an already active semaphore as an error
- `sem_destroy()`
    - Reurns -1 when error occurs
- `sem_wait()`
    - Reurns -1 when error occurs
- `sem_signal()`
    - Reurns -1 when error occurs

#### 3. Solve File Read/Write Mutual Exclusion Using Semaphores
- `file_init()`
    - Doesn't check if the file name is null
- `get_file_cnt()`
    - Doesn't check if the file name is null
    - Since `atoi()` can't handle negative numbers, so the behavior may be unexpected when the file contains a negative number
- `set_file_cnt()`
    - Doesn't check if the file name is null
- `child_proc()`
    - Sleeps for `NUM_CHILDREN` ticks to ensure all child processes are created before proceeding

### Code
#### Fixes
- `pram.h`
    ```c
    //...

    // #define FSSIZE       1000  // size of file system in blocks
    #define FSSIZE       1080  // size of file system in blocks

    ```
    Since more code are added, the file system size needs to be increased to avoid running out of space. (1080 is the minimum size to run the code)

#### 1. Process Priority
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

#### 2. Semaphore Implementation
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

#### 3. Solve File Read/Write Mutual Exclusion Using Semaphores
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
### 1. Process Priority
#### Steps
1. Try `ps` command
2. Try `nice`, `nice 1`, `nice 1 2 3`
3. Try `nice 100 5`, `nice 2 0`, `nice 2 21`
4. Try `nice 2 1`, then `ps` command; `nice 2 20`, then `ps` command; `nice 2 5`, then `ps` command

#### Expected Result
1. Display the process list in the specified format
2. Show usage error message
3. Show error message and invalid parameter message
4. Change the priority of process with PID 2 accordingly

#### Actual Result
```shell
SeaBIOS (version 1.16.3-debian-1.16.3-2)


iPXE (https://ipxe.org) 00:03.0 CA00 PCI2.10 PnP PMM+1EFCAF60+1EF0AF60 CA00
                                                                               


Booting from Hard Disk..xv6...
cpu0: starting 0
sb: size 1080 nblocks 996 ninodes 200 nlog 30 logstart 2 inodestart 32 bmap start 83
Scheduler default policy: DEFAULT
init: starting lsh
$ ps
PID     PPID    PRI     MEM     STATE           CMD
1       N/A     10      12288   SLEEPING        init
2       1       10      16384   SLEEPING        sh
3       2       10      12288   RUNNING         ps
$ nice
nice: usage: nice <pid> <prio>
$ nice 1
nice: usage: nice <pid> <prio>
$ nice 1 2 3
nice: usage: nice <pid> <prio>
$ nice 100 5
nice: error to set pid=100 with prio=5
$ nice 2 0
nice: invalid param(prio=0), should be in [1,20]
$ nice 2 21
nice: invalid param(prio=21), should be in [1,20]
$ nice 2 1
$ ps
PID     PPID    PRI     MEM     STATE           CMD
1       N/A     10      12288   SLEEPING        init
2       1       1       16384   SLEEPING        sh
11      2       10      12288   RUNNING         ps
$ nice 2 20
$ ps
PID     PPID    PRI     MEM     STATE           CMD
1       N/A     10      12288   SLEEPING        init
2       1       20      16384   SLEEPING        sh
13      2       10      12288   RUNNING         ps
$ nice 2 5
$ ps
PID     PPID    PRI     MEM     STATE           CMD
1       N/A     10      12288   SLEEPING        init
2       1       5       16384   SLEEPING        sh
15      2       10      12288   RUNNING         ps
$ 
```

All results are as expected.

### 2. Semaphore Implementation & 3. Solve File Read/Write Mutual Exclusion Using Semaphores
Since the semaphore implementation is tested within `sem_test`, the results of both tasks are presented together.

#### Steps
1. Increase `FSSIZE` (to at least 1081) in `param.h` since file which stores the counter needs space. Then try `sem_test` command
2. Try `ls` and `cat counter` to check the final value in the file

#### Expected Result
1. After all child processes finish, the final value in the file should be 500
2. File `conter` should exist and contain the value 500

#### Actual Result
```shell
SeaBIOS (version 1.16.3-debian-1.16.3-2)


iPXE (https://ipxe.org) 00:03.0 CA00 PCI2.10 PnP PMM+1EFCAF60+1EF0AF60 CA00
                                                                               


Booting from Hard Disk..xv6...
cpu0: starting 0
sb: size 1081 nblocks 997 ninodes 200 nlog 30 logstart 2 inodestart 32 bmap start 83
Scheduler default policy: DEFAULT
init: starting lsh
$ sem_test
sem_test: child started
sem_test: child started
sem_test: child started
sem_test: child started
sem_test: child started
sem_test: child started
sem_test: child started
sem_test: child startesem_test: child started
sem_test: child started
d
sem_test: test passed (cnt=500)
$ ls
.              1 1 512
..             1 1 512
README         2 2 2290
cat            2 3 18828
echo           2 4 17608
forktest       2 5 9988
grep           2 6 22340
init           2 7 18492
kill           2 8 17636
ln             2 9 17552
ls             2 10 20504
mkdir          2 11 17692
rm             2 12 17676
sh             2 13 34524
stressfs       2 14 18620
usertests      2 15 75692
wc             2 16 19352
zombie         2 17 17204
head           2 18 22556
cp             2 19 19496
lsh            2 20 26588
ps             2 21 18928
nice           2 22 18088
scheduler_test 2 23 17272
sem_test       2 24 23704
console        3 25 0
counter        2 26 4
$ cat counter
500
$ 
```

All results are as expected.
