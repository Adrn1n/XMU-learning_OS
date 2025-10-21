# Lab1
## Tasks
### 1
Realize the `cp` command under Linux.
- Command format:
    - `cp [src_file] [dest_file]`
- Steps:
    - complete `cp.c` in the code directory
    - use the function calls `open`, `read`, `write` to handle the source file and destination file

### 2
Realize a shell environment under Linux.
- complete functions `lsh_cd` and `lsh_exit` in `lsh.c`
- extend functionality of `lsh.c` to support redirection and piping (`<`, `>`, `|`)
    - the function `lsh_execute` must be modified accordingly to handle 3 new cases (`REDIN`, `REDOUT`, `PIPE`)
- modify `init.c` to support `lsh`

## Setup
### Environment
- M4 MacBook Air
- VMware Fusion Professional Version 13.6.3 (24585314)
- `Linux ubuntuserver 6.14.0-33-generic #33~24.04.1-Ubuntu SMP PREEMPT_DYNAMIC Fri Sep 19 16:19:58 UTC 2 aarch64 aarch64 aarch64 GNU/Linux`

### Install tools
```shell
sudo apt-get install git qemu-system build-essential 

```

#### Binary utilities
1. Download the [latest version (2.45)](https://ftp.gnu.org/gnu/binutils/binutils-2.45.tar.xz) of the Binutils source code from [GNU ftp](https://ftp.gnu.org/gnu/binutils/) and extract it
2. Create a temporary directory for the out-of-source build and enter it
3. Configure and build (install to `$HOME/opt/cross`):
    ```shell
    ../binutils-2.45/configure --target=i386-jos-elf --prefix=$HOME/opt/cross --disable-nls --disable-werror
    make -j$(nproc)
    make install

    ```

#### GCC
1. Install prerequisites:
    ```shell
    sudo apt install libgmp-dev libmpfr-dev libmpc-dev 

    ```
2. Download the [latest version (15.2.0)](https://ftp.gnu.org/gnu/gcc/gcc-15.2.0/gcc-15.2.0.tar.xz) of the GCC source code from [GNU ftp](https://ftp.gnu.org/gnu/gcc/) and extract it
3. Create a temporary directory for the out-of-source build and enter it
4. Configure and build (install to `$HOME/opt/cross`):
    ```shell
    ../gcc-15.2.0/configure --target=i386-jos-elf --prefix=$HOME/opt/cross --disable-nls --enable-languages=c --without-headers
    # make all-gcc -j$(nproc) # out of memory
    make all-gcc # about 12 min
    make install-gcc

    ```

### Build
1. Set environment variables: `export PATH=$HOME/opt/cross/bin:$PATH`
2. Compile and run:
    ```shell
    make clean
    make
    make qemu-nox

    ```

### Code fixes
#### mp.h
```c
// void *physaddr;               // phys addr of MP config table
uint physaddr;                // phys addr of MP config table

```

To prevent the compiler from treating `physaddr` as a pointer type, which could lead to out-of-bound errors when performing arithmetic operations on it.

#### sh.c
```c
//...
struct cmd *parsecmd(char*);
void runcmd(struct cmd *cmd) __attribute__((noreturn)); //

// Execute cmd.  Never returns.
//...

```

or

`Makefile`
```makefile
#...
#CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -O2 -Wall -MD -ggdb -m32 -Werror -Wno-unused-variable -Wno-return-type  -Wno-implicit-function-declaration -fno-omit-frame-pointer -D $(SCHEDPOLICY)
 CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -O2 -Wall -MD -ggdb -m32 -Werror -Wno-unused-variable -Wno-return-type  -Wno-implicit-function-declaration -fno-omit-frame-pointer -D $(SCHEDPOLICY) -Wno-infinite-recursion
#CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -fvar-tracking -fvar-tracking-assignments -O0 -g -Wall -MD -gdwarf-2 -m32 -Werror -fno-omit-frame-pointer
#...

```

`runcmd()` is designed to recursively call itself and never returns (ends via `exit()` or `exec()`), so the compiler may warn about infinite recursion. Adding `__attribute__((noreturn))` or disabling the warning can resolve this issue.

#### fs.h
```c
// #define NDIRECT 12
#define NDIRECT 28

```

Increase the number of direct blocks in inode structure to increase the maximum file size (28 is just right).

#### param.h
```c
// #define FSSIZE       1000  // size of file system in blocks
#define FSSIZE       1053  // size of file system in blocks

```

Increase the file system size to accommodate all used blocks (1053 is just right at initally, 1062 is just right after all code changes is done).

## Implementation
### 1
#### Features
- Only supports copying regular files
- Does not allow copying a file to itself
- If the destination file exists, it will be overwritten; otherwise, it will be created
- If the destination is a directory (ends with `/`), the source file will be copied into that directory with the same filename

#### Steps
1. Check the number of arguments; if not equal to 3, print usage message and exit
2. Compare source and destination filenames; if they are the same, print error message and exit
3. Open the source file in read-only mode; if it fails, print error message and exit
4. Check if the destination ends with `/`; if so, extract the filename from the source path and append it to the destination path
5. Unlink the destination file to remove it if it exists
6. Open the destination file in create and write-only mode; if it fails, print error message, close the source file, and exit
7. Read from the source file in chunks and write to the destination file until reaching the end of the source file
8. Handle read and write errors
9. Close both files and exit

#### Code
```c
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define BUF_SIZE 512

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf(2, "cp: usage: cp <src> <dst>\n");
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

```

### 2
#### 2.1
##### Features
- `cd [dir]`: return 1 on successful, 0 otherwise
- `exit`: exit the process, return 0 if failed

##### Steps
- `cd`:
    1. Use `chdir()` system call to change the current working directory to the specified path
    2. If `chdir()` fails, print an error message and return 0
    3. If successful, return 1
- `exit`:
    1. Call the `exit()` system call to terminate the shell process
    2. Return 0 (though this line is never reached when successful; if returned, it indicates failure)

##### Code
`lsh.c`
```c
//...
int lsh_cd(struct cmd *cmd)
{
  if (chdir(cmd->left[1]) < 0)
  {
    printf(2, "cd: error access %s\n", cmd->left[1]);
    return 0;
  }
  return 1;
}
//...
int lsh_exit(struct cmd *cmd)
{
  exit();
  return 0;
}
//...

```

#### 2.2
Return 1 on successful execution, 0 otherwise. But can not detect execution failure in child processes.

##### 2.2.1
###### Features
- Only the first input file is used; if multiple input files are specified, a warning is printed
- If the input file cannot be opened, an error message is printed and returns 0
- If command execution fails, an error message is printed

###### Steps
1. Check if multiple input files are specified; if so, print a warning message
2. Attempt to open the specified input file in read-only mode; if it fails, print an error message and return 0
3. Fork a new process to execute the command:
    - In the child process:
        1. Close the standard input (file descriptor 0)
        2. Duplicate the file descriptor of the opened input file to standard input
        3. Close the original file descriptor of the input file
        4. Execute the command using `exec()`
        5. If `exec()` fails, print an error message and exit
    - In the parent process:
        1. Close the file descriptor of the input file
        2. Wait for the child process to finish
4. Return 1

###### Code
`lsh.c`
```c
//...
  case REDIN:
    if (cmd->right[1])
      printf(2, "<: too many inputs, only the first will be used\n");
    if ((fd = open(cmd->right[0], O_RDONLY)) < 0)
    {
      printf(2, "<: failed to open %s\n", cmd->right[0]);
      return 0;
    }
    if (fork() == 0)
    {
      close(0), dup(fd), close(fd);
      exec(cmd->left[0], cmd->left);
      printf(2, "lsh: exec failed\n");
      exit();
    }
    close(fd);
    wait();

    break;
//...

```

##### 2.2.2
###### Features
- Only the first output file is used; if multiple output files are specified, a warning is printed
- If the output file cannot be created/opened, an error message is printed and returns 0
- If command execution fails, an error message is printed

###### Steps
1. Check if multiple output files are specified; if so, print a warning message
2. Unlink the specified output file to remove it if it exists
3. Attempt to open/create the specified output file in write-only mode; if it fails, print an error message and return 0
4. Fork a new process to execute the command:
    - In the child process:
        1. Close the standard output (file descriptor 1)
        2. Duplicate the file descriptor of the opened output file to standard output
        3. Close the original file descriptor of the output file
        4. Execute the command using `exec()`
        5. If `exec()` fails, print an error message and exit
    - In the parent process:
        1. Close the file descriptor of the output file
        2. Wait for the child process to finish
5. Return 1

###### Code
`lsh.c`
```c
//...
  case REDOUT:
    if (cmd->right[1])
      printf(2, ">: too many outputs, only the first will be used\n");
    unlink(cmd->right[0]);
    if ((fd = open(cmd->right[0], O_CREATE | O_WRONLY)) < 0)
    {
      printf(2, ">: failed to open %s\n", cmd->right[0]);
      return 0;
    }
    if (fork() == 0)
    {
      close(1), dup(fd), close(fd);
      exec(cmd->left[0], cmd->left);
      printf(2, "lsh: exec failed\n");
      exit();
    }
    close(fd);
    wait();

    break;
//...

```

##### 2.2.3
###### Features
- If command execution fails in either process, an error message is printed

###### Steps
1. Create a pipe using the `pipe()` system call; if it fails, print an error message and return 0
2. Fork the first child process to execute the left command:
    - In the first child process:
        1. Close the read end of the pipe
        2. Close the standard output (file descriptor 1)
        3. Duplicate the write end of the pipe to standard output
        4. Close the original write end of the pipe
        5. Execute the left command using `exec()`
        6. If `exec()` fails, print an error message and exit
3. Fork the second child process to execute the right command:
    - In the second child process:
        1. Close the write end of the pipe
        2. Close the standard input (file descriptor 0)
        3. Duplicate the read end of the pipe to standard input
        4. Close the original read end of the pipe
        5. Execute the right command using `exec()`
        6. If `exec()` fails, print an error message and exit
4. In the parent process:
    1. Close both ends of the pipe
    2. Wait for both child processes to finish
5. Return 1

###### Code
`lsh.c`
```c
//...
  case PIPE:
    if ((pipe(p)) < 0)
    {
      printf(2, "|: failed to create pipe\n");
      return 0;
    }
    if (fork() == 0)
    {
      close(p[0]), close(1), dup(p[1]), close(p[1]);
      exec(cmd->left[0], cmd->left);
      printf(2, "lsh: exec failed\n");
      exit();
    }
    if (fork() == 0)
    {
      close(p[1]), close(0), dup(p[0]), close(p[0]);
      exec(cmd->right[0], cmd->right);
      printf(2, "lsh: exec failed\n");
      exit();
    }
    close(p[0]), close(p[1]);
    wait(), wait();

    break;
//...

```

#### 2.3
Change the default shell to `lsh` in `init.c` and handle the error message accordingly:
```c
// char *argv[] = {"sh", 0};
char *argv[] = {"lsh", 0};
//...
      // printf(1, "init: exec sh failed\n");
      printf(1, "init: exec %s failed\n", argv[0]);
//...

```

## Results
### 1

### 2
#### 2.1

#### 2.2
##### 2.2.1

##### 2.2.2

##### 2.2.3

#### 2.3
