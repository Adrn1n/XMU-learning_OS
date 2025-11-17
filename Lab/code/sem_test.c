#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

#define NUM_CHILDREN 10
#define TARGET_COUNT_PER_CHILD 50
#define COUNTER_FILE "counter"
#define MAX_NUM_LEN 3
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
