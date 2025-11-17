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

## Results
