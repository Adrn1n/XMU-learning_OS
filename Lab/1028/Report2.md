# Lab2
## Tasks
### 1
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
- `nice pid priority` (priority values are in the range [1, 20], where a smaller value indicates higher priority).
- Implement the system call (`setpriority(pid, priority)`).

## Implementation

## Results
