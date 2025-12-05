# Lab3
## Tasks
**Scheduling Algorithm Comparison**
- Implement the following scheduling methods: Round Robin, Priority, First-Come-First-Served, and Static Multilevel Queue (`proc.c`).
- Testing method (`test_scheduler.c`): Create 6 processes (including CPU-bound and I/O-bound), record the execution process of all processes, and compare the scheduling algorithms' results on average ready time, average running time, average sleep time, and average turnaround time.

## Environment
- M4 MacBook Air
- VMware Fusion Professional Version 13.6.3 (24585314)
- `Linux ubuntuserver 6.14.0-36-generic #36~24.04.1-Ubuntu SMP PREEMPT_DYNAMIC Wed Oct 15 15:22:32 UTC 2 aarch64 aarch64 aarch64 GNU/Linux`

## Implementation
### Fixes
**`param.h`**
```c
// #define FSSIZE       1000  // size of file system in blocks
#define FSSIZE       1102  // size of file system in blocks
```
Since more code are added, the file system size needs to be increased to avoid running out of space. (1102 is the minimum size to run the code)

**`Makefile`**
```makefile
UPROGS=\
	_cat\
	_echo\
	_forktest\
	_grep\
	_init\
	_kill\
	_ln\
	_ls\
	_mkdir\
	_rm\
	_sh\
	_stressfs\
	_usertests\
	_wc\
	_zombie\
	_head\
	_cp\
	_lsh\
	_ps\
	_nice\
# 	_scheduler_test\
# 	_sem_test\

UPROGS+=\
	_test_scheduler\
	_sem_test\
```
Since the file name is required to be `test_scheduler.c`, the corresponding target in the Makefile needs to be changed from the original `_scheduler_test` to `_test_scheduler`.

### Round Robin
#### Features
- Returns 0(NULL) if no `RUNNABLE` process is found

#### Steps
1. Define a static variable `idx` to keep track of the last selected process index.
2. Loop round the process table starting from the next index after `idx`
3. If a `RUNNABLE` process is found, update `idx` and return the process pointer.
4. If no `RUNNABLE` process is found after a full loop, return 0(NULL).

#### Code
`proc.c`
```c
// // CFS Scheduler -------------------------
// RR Scheduler -------------------------
struct proc *rrScheduler() {
  /*
  */
  static int idx=NPROC-1;
  int i=idx;
  do
    if(ptable.proc[(idx=((idx+1)%NPROC))].state==RUNNABLE)
      return ptable.proc+idx;
  while(i!=idx);
  return 0;
}
```

### Priority
#### Features
- Returns 0(NULL) if no `RUNNABLE` process is found
- If multiple `RUNNABLE` processes have the same highest priority, the first one found (lowest index in the process table) is selected

#### Steps
1. Initialize a pointer `res` to keep track of the selected process (initially NULL)
2. Loop through the process table
3. If a `RUNNABLE` process is found, compare its priority with the current `res`
4. If `res` is NULL or the current process has a higher priority (lower numerical value), update `res`
5. After the loop, return `res`

#### Code
`proc.c`
```c
struct proc *priorityScheduler() {
  /*
  */
  struct proc *res=0;
  for(struct proc *p=(ptable.proc);p<(ptable.proc+NPROC);++p)
    if(p->state==RUNNABLE)
    {
      if(res)
      {
        if((p->priority)<(res->priority))
          res=p;
      }
      else
        res=p;
    }
  return res;
}
```

### First-Come-First-Served
#### Features
- Returns 0(NULL) if no `RUNNABLE` process is found
- If multiple `RUNNABLE` processes have the same creation time, the first one found (lowest index in the process table) is selected

#### Steps
**`proc.c`**
1. Initialize a pointer `res` to keep track of the selected process (initially NULL)
2. Loop through the process table
3. If a `RUNNABLE` process is found, compare its creation time with the current `res`
4. If `res` is NULL or the current process has an earlier creation time, update `res`
5. After the loop, return `res`

**`trap.c`**
1. Define a constant array `NON_PREEMPTIVE_SCHED` to hold the scheduler index for non-preemptive scheduling (FCFS(2) in this case)
2. In the timer interrupt handling section, check if the current scheduler is in the `NON_PREEMPTIVE_SCHED` array
3. If it is not, call `yield()` to allow preemption

#### Code
`proc.c`
```c
struct proc *fcfsScheduler() {
  /*
  */
  struct proc *res=0;
  for(struct proc *p=(ptable.proc);p<(ptable.proc+NPROC);++p)
    if(p->state==RUNNABLE)
    {
      if(res)
      {
        if((p->ctime)<(res->ctime))
          res=p;
      }
      else
        res=p;
    }
  return res;
}
```

`trap.c`
```c
/*
*/
#define NON_PREEMPTIVE_SCHED_NUM 1
const int NON_PREEMPTIVE_SCHED[NON_PREEMPTIVE_SCHED_NUM] = {2};

//...

  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    // yield();
    /*
    */
    {
      short flag=1;
      for(int i=0;i<NON_PREEMPTIVE_SCHED_NUM;++i)
        if(getscheduler()==NON_PREEMPTIVE_SCHED[i])
        {
          flag=0;
          break;
        }
      if(flag)
        yield();
    }
```

### Static Multilevel Queue
#### Features
- Returns 0(NULL) if no `RUNNABLE` process is found
- `MLQ_LEVELS` defines the number of priority levels (3 in this case)
- `MLQ_TIME_SLICES` defines the time slices for each level (7, 14, and above 14 for the three levels)
- Used random selection (same as original xv6 implementation) among processes in the same level to avoid starvation. This policy is slightly different from the word *static*

#### Steps
1. Define constants `MLQ_LEVELS` and `MLQ_TIME_SLICES` to set up the multilevel queue parameters
2. Create a 2D array `MQ` to hold `RUNNABLE` processes for each level
3. Create an array `Idxs` to keep track of the number of processes in each level
4. Loop through the process table and categorize `RUNNABLE` processes into the appropriate level
5. After categorization, loop through the levels from highest to lowest priority
6. If a level has `RUNNABLE` processes, randomly select one and return it; otherwise, continue to the next level; if no `RUNNABLE` processes are found in any level, return 0(NULL)

#### Code
```c
// SML Scheduler ---------------------------
/*
*/
#define MLQ_LEVELS 3
const int MLQ_TIME_SLICES[MLQ_LEVELS]={7,14};
struct proc *smlScheduler() {
  /*
  */
  struct proc *MQ[MLQ_LEVELS][NPROC]={0};
  int Idxs[MLQ_LEVELS]={0};
  for(struct proc *p=(ptable.proc);p<(ptable.proc+NPROC);++p)
    if(p->state==RUNNABLE)
    {
      for(int i=0;i<MLQ_LEVELS;++i)
        if(i<(MLQ_LEVELS-1))
        {
          if(p->priority<=(MLQ_TIME_SLICES[i]))
          {
            MQ[i][Idxs[i]++]=p;
            break;
          }
        }
        else
          MQ[i][Idxs[i]++]=p;
    }
  for(int i=0;i<MLQ_LEVELS;++i)
    if(Idxs[i]>0)
      return MQ[i][random(Idxs[i])];
  return 0;
}
```

### Testing Method
#### Features
- Since the `schedulerName` in `sdh.h` is not changed from `CFS` to `RR`, so the results will still show `CFS` instead of `RR`
- Since `printf()` in xv6 doesn't support floating point numbers, the average values are displayed as total values divided by the number of processes and are without reduction

For each scheduling algorithm:
- Create 2 kinds of tasks: CPU-bound and I/O-bound
- Each kind of task has 3 processes with different priorities (7, 14, 20), which means a total of 6 processes as required
- Record the execution metrics (ready time, running time, sleep time) for each

For CPU-bound task:
- CPU-bound task: perform calculations (multiplications in this case) for `cal_len` iterations, repeated `cal_rnd` times

For I/O-bound task:
- I/O-bound task: write a line of length `wr_len` to a file `f_name`, repeated `wr_num` times
- If file open fails, print an error message to standard error

For test runner:
- The number of processes for each task type is defined in `Num_tsk` array
- The length of `Num_tsk` (number of task types) is defined by `len`
- The results are stored in `Res` array, with each process having `MONI_NUM` metrics (3: ready time, running time, sleep time); And is flattened as a 1D array, in the order of task type, process number, and metric type
- The priorities for each process are defined in `Prio` array; And is flattened as a 1D array, in the order of task type and process number
- Task number and the corresponding task type mapping:
    - 0: CPU-bound task
    - 1: I/O-bound task
    - Other numbers: invalid task type
- Sleep for `TSK_LEN` ticks before starting the actual task to ensure all processes are created before execution
- If execution of a process fails (fork, or invalid task type), the corresponding metrics in `Res` are set to -1 and an error message is printed to standard error
- If setting priority fails, an error message is printed to standard error and execution continues with the default priority
- If waiting for a process fails, an error message is printed to standard error; And the corresponding metrics in `Res` which it should update are not updated
- If the returned process ID from `wait2()` cannot be found in the mapping, an error message is printed to standard error; And the corresponding metrics in `Res` which it should update are not updated

#### Steps
Based on the code, here are the refined detailed steps:

#### Steps
1. Define constants for scheduler count, process count, task counts, monitoring metrics, CPU task parameters, I/O task parameters, file name, priority array, and sleep time
2. Implement `tast_cpu()` function to perform CPU-bound calculations
    1. Validate `cal_len` and `cal_rnd` are positive, otherwise exit
    2. Execute nested loops with conditional multiplication operations
    3. Call `exit()` after completion
3. Implement `tast_io()` function to perform I/O-bound file writing
    1. Validate parameters are valid, otherwise exit
    2. Loop `wr_num` times: open file, write character pattern if successful (print error if failed), close file
    3. Call `exit()` after completion
4. Implement `run_test()` function to create processes, set priorities, and collect execution metrics
    1. Validate input parameters, return 0 if invalid
    2. Initialize task counter, PID-to-result mapping array, and process index
    3. Loop through each task type and print testing information
    4. Use switch statement on task type:
        - Case 0 (CPU-bound): Loop for specified number, fork child (sleep then execute `tast_cpu()`), parent sets priority and stores mapping, handle errors
        - Case 1 (I/O-bound): Loop for specified number, fork child (sleep then execute `tast_io()`), parent sets priority and stores mapping, handle errors
        - Default: Print error and set metrics to -1
    5. Loop to wait for all created processes using `wait2()` to collect metrics
    6. Find returned PID in mapping array and store results in `Res` array
    7. Handle errors by printing to stderr if `wait2()` fails or PID not found
    8. Return total count of created processes
5. In `main()` function, declare 3D array for results and save current scheduler
6. Loop through each scheduler: print testing message, switch scheduler, call `run_test()` with parameters
7. After testing all schedulers, restore original scheduler and print completion message
8. For each scheduler, loop through processes to accumulate metrics (skip if error), then print average metrics as unreduced fractions (ready time, running time, sleep time, turnaround time)
9. Call `exit()` to terminate

#### Code
`test_scheduler.c`
```c
/*
 */
#define SCHED_NUM 5

/*
 */
#define PROC_NUM 6
#define TSK_LEN 2
#define TSK1_NUM 3
#define TSK2_NUM 3
#define MONI_NUM 3
#define CPU_CAL_LEN 1000
#define CPU_CAL_RND 1000
#define WR_LEN 100
#define WR_RND 100

/*
 */
char *IO_FILE_NAME = "tstSchedIO.txt";
const int TSK_PRIO[PROC_NUM] = {7, 14, 20, 7, 14, 20};

/*
 */
void tast_cpu(const int cal_len, const int cal_rnd)
{
    if ((cal_len > 0) && (cal_rnd > 0))
    {
        for (int i = 0; i < cal_rnd; ++i)
        {
            int temp = 1;
            for (int j = 0; j < cal_len; ++j)
                temp *= (i % 2) ? (j + i) : (j - i);
        }
    }
    exit();
}

/*
 */
void tast_io(char *const f_name, const int wr_len, const int wr_num)
{
    if (f_name && (wr_len > 0) && (wr_num > 0))
    {
        char line[wr_len + 1];
        for (int i = 0; i < wr_num; ++i)
        {
            int fd = open(f_name, O_CREATE | O_RDWR);
            if (fd < 0)
                printf(2, "test_io(): ERROR: Open file %s failed\n", f_name);
            else
            {
                for (char *p = line; p < (line + wr_len); ++p)
                    *p = (i % 2) ? ('A' + (p - line + i) % 26) : ('A' + (p - line - i) % 26);
                line[wr_len] = 0;
                printf(fd, "%s\n", line);
            }
            close(fd);
        }
    }
    exit();
}

/*
 */
int run_test(const int *const Num_tsk, const int len, int *const Res, const int *const Prio)
{
    int tsk_cnt = 0;
    if (Num_tsk && (len > 0) && Res && Prio)
    {
        int Map_PID2Res[PROC_NUM][2] = {0}, idx = 0;
        for (int i = 0; i < len; ++i)
        {
            printf(1, "run_test(): Testing task %d, number %d\n", i, Num_tsk[i]);
            switch (i)
            {
            case 0:
                for (int j = 0; j < Num_tsk[i]; ++j)
                {
                    int pid = fork();
                    if (pid < 0)
                    {
                        printf(2, "run_test(): ERROR: fork() failed for %d task %d\n", i, j);
                        for (int k = 0; k < MONI_NUM; ++k)
                            Res[MONI_NUM * idx + k] = -1;
                    }
                    else if (pid == 0)
                    {
                        sleep(TSK_LEN);
                        tast_cpu(CPU_CAL_LEN, CPU_CAL_RND);
                    }
                    else
                    {
                        if (setpriority(pid, Prio[idx]) < 0)
                            printf(2, "run_test(): ERROR: setpriority() failed for %d task %d ,pid %d\n", i, j, pid);
                        Map_PID2Res[tsk_cnt][0] = pid, Map_PID2Res[tsk_cnt++][1] = idx;
                    }
                    ++idx;
                }
                break;
            case 1:
                for (int j = 0; j < Num_tsk[i]; ++j)
                {
                    int pid = fork();
                    if (pid < 0)
                    {
                        for (int k = 0; k < MONI_NUM; ++k)
                            Res[MONI_NUM * idx + k] = -1;
                        printf(2, "run_test(): ERROR: fork() failed for %d task %d\n", i, j);
                    }
                    else if (pid == 0)
                    {
                        sleep(TSK_LEN);
                        tast_io(IO_FILE_NAME, WR_LEN, WR_RND);
                    }
                    else
                    {
                        if (setpriority(pid, Prio[idx]) < 0)
                            printf(2, "run_test(): ERROR: setpriority() failed for %d task %d ,pid %d\n", i, j, pid);
                        Map_PID2Res[tsk_cnt][0] = pid, Map_PID2Res[tsk_cnt++][1] = idx;
                    }
                    ++idx;
                }
                break;
            default:
                printf(2, "run_test(): ERROR: Unknown task(%d)\n", i);
                for (int j = 0; j < Num_tsk[i]; ++j, ++idx)
                    for (int k = 0; k < MONI_NUM; ++k)
                        Res[idx + k] = -1;
            }
        }
        for (int i = 0; i < tsk_cnt; ++i)
        {
            int res[MONI_NUM] = {0}, pid = wait2(res, res + 1, res + 2);
            if (pid < 0)
                printf(2, "run_test(): ERROR: wait2() failed\n");
            else
            {
                int res_idx = -1;
                for (int j = 0; j < tsk_cnt; ++j)
                    if (Map_PID2Res[j][0] == pid)
                    {
                        res_idx = Map_PID2Res[j][1], Res[MONI_NUM * res_idx + 0] = res[0], Res[MONI_NUM * res_idx + 1] = res[1], Res[MONI_NUM * res_idx + 2] = res[2];
                        break;
                    }
                if (res_idx < 0)
                    printf(2, "run_test(): ERROR: cannot find pid %d in map\n", pid);
            }
        }
    }
    return tsk_cnt;
}

int main(int argc, char *argv[])
{
    /*
     */
    int sched_num = SCHED_NUM, monitor_time[sched_num][PROC_NUM][MONI_NUM], cur_sched = getscheduler();
    for (int sched = 0; sched < sched_num; ++sched)
    {
        printf(1, "test_scheduler: Testing scheduler %s\n", schedulerName[sched]);
        setscheduler(sched);
        run_test((int[]){TSK1_NUM, TSK2_NUM}, TSK_LEN, monitor_time[sched][0], TSK_PRIO);
    }
    setscheduler(cur_sched);
    printf(1, "test_scheduler: Testing completed\n");
    for (int sched = 0; sched < sched_num; ++sched)
    {
        printf(1, "Results for scheduler %s:\n", schedulerName[sched]);
        int cnt = 0;
        int Res[MONI_NUM] = {0};
        for (int tsk = 0; tsk < PROC_NUM; ++tsk)
            if (monitor_time[sched][tsk][0] < 0)
            {
                printf(1, "\tTask %d: ERROR in execution\n", tsk);
                break;
            }
            else
            {
                for (int m = 0; m < MONI_NUM; ++m)
                    Res[m] += monitor_time[sched][tsk][m];
                ++cnt;
            }
        printf(1, "\tAverage ready time: %d/%d\n", Res[0], cnt);
        printf(1, "\tAverage running time: %d/%d\n", Res[1], cnt);
        printf(1, "\tAverage sleep time: %d/%d\n", Res[2], cnt);
        printf(1, "\tAverage turnaround time: %d/%d\n", Res[0] + Res[1] + Res[2], cnt);
    }
    exit();
}

```

## Results
Since the scheduling implementation is tested within `test_scheduler.c`, the results of both tasks are combined here.

### Steps
Increase FSSIZE (to at least 1103) in param.h since I/O processes need to write to the file system. Then try `test_scheduler` command.

### Expected Result
All scheduling algorithms should be executed without errors, and the average times should be displayed.

### Actual Result
```shell
SeaBIOS (version 1.16.3-debian-1.16.3-2)


iPXE (https://ipxe.org) 00:03.0 CA00 PCI2.10 PnP PMM+1EFCAF60+1EF0AF60 CA00
                                                                               


Booting from Hard Disk..xv6...
cpu0: starting 0
sb: size 1103 nblocks 1019 ninodes 200 nlog 30 logstart 2 inodestart 32 bmap start 83
Scheduler default policy: DEFAULT
init: starting lsh
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
test_scheduler 2 23 28516
sem_test       2 24 23704
console        3 25 0
$ test_scheduler
test_scheduler: Testing scheduler DEFAULT
run_test(): Testing task 0, number 3
run_test(): Testing task 1, number 3
test_scheduler: Testing scheduler PRIORITY
run_test(): Testing task 0, number 3
run_test(): Testing task 1, number 3
test_scheduler: Testing scheduler FCFS
run_test(): Testing task 0, number 3
run_test(): Testing task 1, number 3
test_scheduler: Testing scheduler CFS
run_test(): Testing task 0, number 3
run_test(): Testing task 1, number 3
test_scheduler: Testing scheduler SML
run_test(): Testing task 0, number 3
run_test(): Testing task 1, number 3
test_scheduler: Testing completed
Results for scheduler DEFAULT:
        Average ready time: 1142/6
        Average running time: 762/6
        Average sleep time: 914/6
        Average turnaround time: 2818/6
Results for scheduler PRIORITY:
        Average ready time: 2957/6
        Average running time: 1266/6
        Average sleep time: 4225/6
        Average turnaround time: 8448/6
Results for scheduler FCFS:
        Average ready time: 1844/6
        Average running time: 1081/6
        Average sleep time: 2340/6
        Average turnaround time: 5265/6
Results for scheduler CFS:
        Average ready time: 1238/6
        Average running time: 727/6
        Average sleep time: 1856/6
        Average turnaround time: 3821/6
Results for scheduler SML:
        Average ready time: 2793/6
        Average running time: 1210/6
        Average sleep time: 3935/6
        Average turnaround time: 7938/6
$
```

### Conclusion
All results are as expected.
