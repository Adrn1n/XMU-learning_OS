#include "types.h"
#include "user.h"
#include "sdh.h"
#include "fcntl.h"

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
    if ((f_name) && (wr_len > 0) && (wr_num > 0))
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
    if ((Num_tsk) && (len > 0) && (Res))
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
