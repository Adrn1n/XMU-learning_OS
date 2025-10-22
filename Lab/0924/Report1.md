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

Increase the file system size to accommodate all used blocks (1053 is just right at initally, 1063 is just right after all code changes is done).

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
- `cd [dir]`: only the first argument is used; return 1 on successful, 0 otherwise
- `exit`: exit the process, return 0 if failed

##### Steps
- `cd`:
    1. Check if multiple arguments are provided; if so, print a warning message
    2. Use `chdir()` system call to change the current working directory to the specified path
    3. If `chdir()` fails, print an error message and return 0
    4. If successful, return 1
- `exit`:
    1. Call the `exit()` system call to terminate the shell process
    2. Return 0 (though this line is never reached when successful; if returned, it indicates failure)

##### Code
`lsh.c`
```c
//...
int lsh_cd(struct cmd *cmd)
{
  if (cmd->left[2])
    printf(2, "cd: too many arguments, only the first will be used\n");
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

Built-in commands are not supported with redirection or piping.

##### 2.2.1
###### Features
- Only the first input file is used

###### Steps
1. Check if multiple input files are specified; if so, print a warning message
2. Attempt to open the specified input file in read-only mode; if it fails, print an error message and return
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
- Only the first output file is used

###### Steps
1. Check if multiple output files are specified; if so, print a warning message
2. Unlink the specified output file to remove it if it exists
3. Attempt to open/create the specified output file in write-only mode; if it fails, print an error message and return
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
1. Create a pipe using the `pipe()` system call; if it fails, print an error message and return
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
Modify `param.h` to increase blocks for the file system size to accommodate all changes.

### 1
#### Steps
1. Try various incorrect usages of `cp` to test error handling
2. Copy `README` to `a.txt`
3. Copy `a.txt` to `a.txt`
4. Create directory `dir` and copy `a.txt` to `dir/`
5. Copy `a.txt` to `dir/b.txt`
6. Copy `.` to `a.txt`

#### Expected results
1. Error messages printed for incorrect usages
2. `a.txt` is created and has the same content as `README`
3. Error message printed: `cp: src(a.txt) and dst(a.txt) are the same`
4. `dir/a.txt` is created and has the same content as `a.txt`
5. `dir/b.txt` is created and has the same content as `a.txt`
6. `a.txt` is overwritten and has the same content as `.`

#### Actual results
```shell
SeaBIOS (version 1.16.3-debian-1.16.3-2)


iPXE (https://ipxe.org) 00:03.0 CA00 PCI2.10 PnP PMM+1EFCAF60+1EF0AF60 CA00
                                                                               


Booting from Hard Disk..xv6...
cpu0: starting 0
sb: size 2063 nblocks 1979 ninodes 200 nlog 30 logstart 2 inodestart 32 bmap start 83
Scheduler default policy: DEFAULT
init: starting lsh
$ ls
.              1 1 512
..             1 1 512
README         2 2 2290
cat            2 3 18868
echo           2 4 17648
forktest       2 5 10008
grep           2 6 22380
init           2 7 18532
kill           2 8 17676
ln             2 9 17592
ls             2 10 20544
mkdir          2 11 17732
rm             2 12 17716
sh             2 13 34564
stressfs       2 14 18660
usertests      2 15 75732
wc             2 16 19392
zombie         2 17 17244
head           2 18 22596
cp             2 19 19536
lsh            2 20 26628
ps             2 21 17056
nice           2 22 17044
scheduler_test 2 23 17312
sem_test       2 24 17080
console        3 25 0
$ 
```

1. 
    ```shell
    $ cp 
    cp: usage: cp <src> <dst>
    $ cp README a.txt b.txt
    cp: usage: cp <src> <dst>
    $ 
    ```
2. 
    ```shell
    $ cat README
    xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
    Version 6 (v6).  xv6 loosely follows the structure and style of v6,
    but is implemented for a modern x86-based multiprocessor using ANSI C.

    ACKNOWLEDGMENTS

    xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
    to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
    2000)). See also http://pdos.csail.mit.edu/6.828/2016/xv6.html, which
    provides pointers to on-line resources for v6.

    xv6 borrows code from the following sources:
        JOS (asm.h, elf.h, mmu.h, bootasm.S, ide.c, console.c, and others)
        Plan 9 (entryother.S, mp.h, mp.c, lapic.c)
        FreeBSD (ioapic.c)
        NetBSD (console.c)

    The following people have made contributions: Russ Cox (context switching,
    locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
    Clements.

    We are also grateful for the bug reports and patches contributed by Silas
    Boyd-Wickizer, Anton Burtsev, Cody Cutler, Mike CAT, Tej Chajed, Nelson Elhage,
    Saar Ettinger, Alice Ferrazzi, Nathaniel Filardo, Peter Froehlich, Yakir Goaron,
    Shivam Handa, Bryan Henry, Jim Huang, Alexander Kapshuk, Anders Kaseorg,
    kehao95, Wolfgang Keller, Eddie Kohler, Austin Liew, Imbar Marinescu, Yandong
    Mao, Hitoshi Mitake, Carmi Merimovich, Joel Nider, Greg Price, Ayan Shafqat,
    Eldar Sehayek, Yongming Shen, Cam Tenny, Rafael Ubal, Warren Toomey, Stephen Tu,
    Pablo Ventura, Xi Wang, Keiichi Watanabe, Nicolas Wolovick, Grant Wu, Jindong
    Zhang, Icenowy Zheng, and Zou Chang Wei.

    The code in the files that constitute xv6 is
    Copyright 2006-2016 Frans Kaashoek, Robert Morris, and Russ Cox.

    ERROR REPORTS

    Please send errors and suggestions to Frans Kaashoek and Robert Morris
    (kaashoek,rtm@mit.edu). The main purpose of xv6 is as a teaching
    operating system for MIT's 6.828, so we are more interested in
    simplifications and clarifications than new features.

    BUILDING AND RUNNING XV6

    To build xv6 on an x86 ELF machine (like Linux or FreeBSD), run
    "make". On non-x86 or non-ELF machines (like OS X, even on x86), you
    will need to install a cross-compiler gcc suite capable of producing
    x86 ELF binaries. See http://pdos.csail.mit.edu/6.828/2016/tools.html.
    Then run "make TOOLPREFIX=i386-jos-elf-". Now install the QEMU PC
    simulator and run "make qemu".
    $ cp README a.txt
    $ ls 
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    a.txt          2 26 2290
    $ cat a.txt
    xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
    Version 6 (v6).  xv6 loosely follows the structure and style of v6,
    but is implemented for a modern x86-based multiprocessor using ANSI C.

    ACKNOWLEDGMENTS

    xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
    to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
    2000)). See also http://pdos.csail.mit.edu/6.828/2016/xv6.html, which
    provides pointers to on-line resources for v6.

    xv6 borrows code from the following sources:
        JOS (asm.h, elf.h, mmu.h, bootasm.S, ide.c, console.c, and others)
        Plan 9 (entryother.S, mp.h, mp.c, lapic.c)
        FreeBSD (ioapic.c)
        NetBSD (console.c)

    The following people have made contributions: Russ Cox (context switching,
    locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
    Clements.

    We are also grateful for the bug reports and patches contributed by Silas
    Boyd-Wickizer, Anton Burtsev, Cody Cutler, Mike CAT, Tej Chajed, Nelson Elhage,
    Saar Ettinger, Alice Ferrazzi, Nathaniel Filardo, Peter Froehlich, Yakir Goaron,
    Shivam Handa, Bryan Henry, Jim Huang, Alexander Kapshuk, Anders Kaseorg,
    kehao95, Wolfgang Keller, Eddie Kohler, Austin Liew, Imbar Marinescu, Yandong
    Mao, Hitoshi Mitake, Carmi Merimovich, Joel Nider, Greg Price, Ayan Shafqat,
    Eldar Sehayek, Yongming Shen, Cam Tenny, Rafael Ubal, Warren Toomey, Stephen Tu,
    Pablo Ventura, Xi Wang, Keiichi Watanabe, Nicolas Wolovick, Grant Wu, Jindong
    Zhang, Icenowy Zheng, and Zou Chang Wei.

    The code in the files that constitute xv6 is
    Copyright 2006-2016 Frans Kaashoek, Robert Morris, and Russ Cox.

    ERROR REPORTS

    Please send errors and suggestions to Frans Kaashoek and Robert Morris
    (kaashoek,rtm@mit.edu). The main purpose of xv6 is as a teaching
    operating system for MIT's 6.828, so we are more interested in
    simplifications and clarifications than new features.

    BUILDING AND RUNNING XV6

    To build xv6 on an x86 ELF machine (like Linux or FreeBSD), run
    "make". On non-x86 or non-ELF machines (like OS X, even on x86), you
    will need to install a cross-compiler gcc suite capable of producing
    x86 ELF binaries. See http://pdos.csail.mit.edu/6.828/2016/tools.html.
    Then run "make TOOLPREFIX=i386-jos-elf-". Now install the QEMU PC
    simulator and run "make qemu".
    $ 
    ```
3. 
    ```shell
    $ cp a.txt a.txt
    cp: src(a.txt) and dst(a.txt) are the same
    $ 
    ```
4. 
    ```shell
    $ mkdir dir
    $ ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    a.txt          2 26 2290
    dir            1 27 32
    $ ls dir
    .              1 27 32
    ..             1 1 512
    $ cp a.txt dir/
    $ ls dir
    .              1 27 48
    ..             1 1 512
    a.txt          2 28 2290
    $ cat dir/a.txt
    xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
    Version 6 (v6).  xv6 loosely follows the structure and style of v6,
    but is implemented for a modern x86-based multiprocessor using ANSI C.

    ACKNOWLEDGMENTS

    xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
    to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
    2000)). See also http://pdos.csail.mit.edu/6.828/2016/xv6.html, which
    provides pointers to on-line resources for v6.

    xv6 borrows code from the following sources:
        JOS (asm.h, elf.h, mmu.h, bootasm.S, ide.c, console.c, and others)
        Plan 9 (entryother.S, mp.h, mp.c, lapic.c)
        FreeBSD (ioapic.c)
        NetBSD (console.c)

    The following people have made contributions: Russ Cox (context switching,
    locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
    Clements.

    We are also grateful for the bug reports and patches contributed by Silas
    Boyd-Wickizer, Anton Burtsev, Cody Cutler, Mike CAT, Tej Chajed, Nelson Elhage,
    Saar Ettinger, Alice Ferrazzi, Nathaniel Filardo, Peter Froehlich, Yakir Goaron,
    Shivam Handa, Bryan Henry, Jim Huang, Alexander Kapshuk, Anders Kaseorg,
    kehao95, Wolfgang Keller, Eddie Kohler, Austin Liew, Imbar Marinescu, Yandong
    Mao, Hitoshi Mitake, Carmi Merimovich, Joel Nider, Greg Price, Ayan Shafqat,
    Eldar Sehayek, Yongming Shen, Cam Tenny, Rafael Ubal, Warren Toomey, Stephen Tu,
    Pablo Ventura, Xi Wang, Keiichi Watanabe, Nicolas Wolovick, Grant Wu, Jindong
    Zhang, Icenowy Zheng, and Zou Chang Wei.

    The code in the files that constitute xv6 is
    Copyright 2006-2016 Frans Kaashoek, Robert Morris, and Russ Cox.

    ERROR REPORTS

    Please send errors and suggestions to Frans Kaashoek and Robert Morris
    (kaashoek,rtm@mit.edu). The main purpose of xv6 is as a teaching
    operating system for MIT's 6.828, so we are more interested in
    simplifications and clarifications than new features.

    BUILDING AND RUNNING XV6

    To build xv6 on an x86 ELF machine (like Linux or FreeBSD), run
    "make". On non-x86 or non-ELF machines (like OS X, even on x86), you
    will need to install a cross-compiler gcc suite capable of producing
    x86 ELF binaries. See http://pdos.csail.mit.edu/6.828/2016/tools.html.
    Then run "make TOOLPREFIX=i386-jos-elf-". Now install the QEMU PC
    simulator and run "make qemu".
    $ 
    ```
5. 
    ```shell
    $ cp a.txt dir/b.txt
    $ ls dir  
    .              1 27 64
    ..             1 1 512
    a.txt          2 28 2290
    b.txt          2 29 2290
    $ cat dir/b.txt
    xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
    Version 6 (v6).  xv6 loosely follows the structure and style of v6,
    but is implemented for a modern x86-based multiprocessor using ANSI C.

    ACKNOWLEDGMENTS

    xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
    to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
    2000)). See also http://pdos.csail.mit.edu/6.828/2016/xv6.html, which
    provides pointers to on-line resources for v6.

    xv6 borrows code from the following sources:
        JOS (asm.h, elf.h, mmu.h, bootasm.S, ide.c, console.c, and others)
        Plan 9 (entryother.S, mp.h, mp.c, lapic.c)
        FreeBSD (ioapic.c)
        NetBSD (console.c)

    The following people have made contributions: Russ Cox (context switching,
    locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
    Clements.

    We are also grateful for the bug reports and patches contributed by Silas
    Boyd-Wickizer, Anton Burtsev, Cody Cutler, Mike CAT, Tej Chajed, Nelson Elhage,
    Saar Ettinger, Alice Ferrazzi, Nathaniel Filardo, Peter Froehlich, Yakir Goaron,
    Shivam Handa, Bryan Henry, Jim Huang, Alexander Kapshuk, Anders Kaseorg,
    kehao95, Wolfgang Keller, Eddie Kohler, Austin Liew, Imbar Marinescu, Yandong
    Mao, Hitoshi Mitake, Carmi Merimovich, Joel Nider, Greg Price, Ayan Shafqat,
    Eldar Sehayek, Yongming Shen, Cam Tenny, Rafael Ubal, Warren Toomey, Stephen Tu,
    Pablo Ventura, Xi Wang, Keiichi Watanabe, Nicolas Wolovick, Grant Wu, Jindong
    Zhang, Icenowy Zheng, and Zou Chang Wei.

    The code in the files that constitute xv6 is
    Copyright 2006-2016 Frans Kaashoek, Robert Morris, and Russ Cox.

    ERROR REPORTS

    Please send errors and suggestions to Frans Kaashoek and Robert Morris
    (kaashoek,rtm@mit.edu). The main purpose of xv6 is as a teaching
    operating system for MIT's 6.828, so we are more interested in
    simplifications and clarifications than new features.

    BUILDING AND RUNNING XV6

    To build xv6 on an x86 ELF machine (like Linux or FreeBSD), run
    "make". On non-x86 or non-ELF machines (like OS X, even on x86), you
    will need to install a cross-compiler gcc suite capable of producing
    x86 ELF binaries. See http://pdos.csail.mit.edu/6.828/2016/tools.html.
    Then run "make TOOLPREFIX=i386-jos-elf-". Now install the QEMU PC
    simulator and run "make qemu".
    $ 
    ```
6. 
    ```shell
    $ cat .
    ...READMEcatechoforktestgrepinikill     ln
    ls
    mkdir
    shstressfsusertestswczombieheadcplshpsnicescheduler_testsem_testconsolea.txtir$ cp . a.txt
    $ cat a.txt
    ...READMEcatechoforktestgrepinikill     ln
    ls
    mkdir
    shstressfsusertestswczombieheadcplshpsnicescheduler_testsem_testconsolea.txtir$ 
    ```

All operations performed as expected.

### 2
#### 2.1
##### Steps
1. Change shell to `lsh`, try a wrong usage and a non-existent directory
2. Create a new directory `dir` and copy `README` into it
3. Change directory to `dir`
4. Change directory to parent directory
5. Exit the `lsh`

##### Expected results
1. Error messages printed for incorrect usages and non-existent directory
2. `dir` is created and contains `README`
3. Current working directory changes to `dir`
4. Current working directory changes back to parent directory
5. Shell exits to `sh`

##### Actual results
1. 
    ```shell
    SeaBIOS (version 1.16.3-debian-1.16.3-2)


    iPXE (https://ipxe.org) 00:03.0 CA00 PCI2.10 PnP PMM+1EFCAF60+1EF0AF60 CA00
                                                                                


    Booting from Hard Disk..xv6...
    cpu0: starting 0
    sb: size 2063 nblocks 1979 ninodes 200 nlog 30 logstart 2 inodestart 32 bmap start 83
    Scheduler default policy: DEFAULT
    init: starting lsh
    $ lsh
    lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    lsh> cd
    cd: error access (null)
    lsh> cd dir
    cd: error access dir
    lsh> cd dir1 dir2
    cd: too many arguments, only the first will be used
    cd: error access dir1
    lsh> 
    ```
2. 
    ```shell
    lsh> mkdir dir 
    lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    dir            1 26 32
    lsh> ls dir
    .              1 26 32
    ..             1 1 512
    lsh> cp README dir/
    lsh> ls dir
    .              1 26 48
    ..             1 1 512
    README         2 27 2290
    lsh> 
    ```
3. 
    ```shell
    lsh> cd dir
    lsh> ../ls
    .              1 26 48
    ..             1 1 512
    README         2 27 2290
    lsh> 
    ```
4. 
    ```shell
    lsh> cd .. dir
    cd: too many arguments, only the first will be used
    lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    dir            1 26 48
    lsh> 
    ```
5. 
    ```shell
    lsh> exit 
    $ 
    ```

All operations performed as expected.

#### 2.2
##### 2.2.1
###### Steps
1. Edit `README` to only containing `ls` and change shell to `lsh`
2. Try `sh <`, `sh < a`, `sh < a README` and `sh < ls`
3. Try `lsh < README` and `sh < README a`
4. Create a directory `dir`, copy `README` into it, and try `sh < dir/README`

###### Expected results
1. `README` is modified to contain `ls`
2. Error messages printed
3. `ls` output by `sh` is displayed; and error message printed
4. `ls` output by `sh` is displayed

###### Actual results
1. 
    ```shell
    SeaBIOS (version 1.16.3-debian-1.16.3-2)


    iPXE (https://ipxe.org) 00:03.0 CA00 PCI2.10 PnP PMM+1EFCAF60+1EF0AF60 CA00
                                                                                


    Booting from Hard Disk..xv6...
    cpu0: starting 0
    sb: size 2063 nblocks 1979 ninodes 200 nlog 30 logstart 2 inodestart 32 bmap start 83
    Scheduler default policy: DEFAULT
    init: starting lsh
    $ lsh
    lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    lsh> cat README
    lslsh> 
    ```
2. 
    ```shell
    lslsh> sh <
    <: failed to open (null)
    lsh> sh < a
    <: failed to open a
    lsh> sh < a README
    <: too many inputs, only the first will be used
    <: failed to open a
    lsh> sh < ls
    $ exec: fail
    exec ELF failed
    $ lsh> 
    ```
3. 
    ```shell
    lsh> sh < README
    $ .              1 1 512
    ..             1 1 512
    README         2 2 2
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    $ lsh> sh < README a
    <: too many inputs, only the first will be used
    $ .              1 1 512
    ..             1 1 512
    README         2 2 2
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    $ lsh> 
    ```
4. 
    ```shell
    $ lsh> mkdir dir
    lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    dir            1 26 32
    lsh> ls dir
    .              1 26 32
    ..             1 1 512
    lsh> cp README dir/
    lsh> ls dir
    .              1 26 48
    ..             1 1 512
    README         2 27 2
    lsh> sh < dir/README
    $ .              1 1 512
    ..             1 1 512
    README         2 2 2
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    dir            1 26 48
    $ lsh> 
    ```

All operations performed as expected.

##### 2.2.2
###### Steps
1. Change shell to `lsh` and try `ls >`
2. Try `ls > dir/a.txt`, `README > a.txt` and `ls > a.txt b.txt`
3. Create directory `dir` and try `ls > dir/a.txt`
3. Try `echo hello > a.txt`
5. Try `help > b.txt`

###### Expected results
1. Some undefined behavior or error message printed (since the next value of memory 0 may not be 0, so the string at `NULL` may not be treated as empty in some built-in functions)
2. Error message printed for `ls > dir/a.txt` since `dir` does not exist; error message printed for `README > a.txt`; `a.txt` is created as empty for `README > a.txt`; error message printed for `ls > a.txt b.txt` and `a.txt` replaced with `ls` output
3. `dir` is created and `a.txt` is created inside it with `ls` output
4. `a.txt` is overwritten with `hello`
5. Help message printed but not written to `b.txt` since `help` is a built-in command

###### Actual results
1. 
    ```shell
    SeaBIOS (version 1.16.3-debian-1.16.3-2)


    iPXE (https://ipxe.org) 00:03.0 CA00 PCI2.10 PnP PMM+1EFCAF60+1EF0AF60 CA00
                                                                                


    Booting from Hard Disk..xv6...
    cpu0: starting 0
    sb: size 2063 nblocks 1979 ninodes 200 nlog 30 logstart 2 inodestart 32 bmap start 83
    Scheduler default policy: DEFAULT
    init: starting lsh
    $ lsh
    lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    lsh> ls >
    lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    �L$����q�U��W 2 26 681
    lsh> 
    ```
2. 
    ```shell
    lsh> ls > dir/a.txt
    >: failed to open dir/a.txt
    lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    �L$����q�U��W 2 26 681
    lsh> README > a.txt
    lsh: exec failed
    lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    �L$����q�U��W 2 26 681
    a.txt          2 27 0
    lsh> cat a.txt
    lsh> ls > a.txt b.txt
    >: too many outputs, only the first will be used
    lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    �L$����q�U��W 2 26 681
    a.txt          2 27 705
    lsh> cat a.txt
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    �L$����q�U��W 2 26 681
    a.txt          2 27 681
    lsh> 
    ```
3. 
    ```shell
    lsh> mkdir dir
    lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    �L$����q�U��W 2 26 681
    a.txt          2 27 705
    dir            1 28 32
    lsh> ls dir
    .              1 28 32
    ..             1 1 512
    lsh> ls > dir/a.txt
    lsh> ls dir
    .              1 28 48
    ..             1 1 512
    a.txt          2 29 728
    lsh> cat dir/a.txt
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    �L$����q�U��W 2 26 681
    a.txt          2 27 705
    dir            1 28 48
    lsh> 
    ```
4. 
    ```shell
    lsh> echo hello > a.txt
    lsh> cat a.txt
    hello
    lsh>  
    ```
5. 
    ```shell
    lsh> help > b.txt
    xv6 LSH
    Type program names and arguments, and hit enter.
    The following are built in:
    cd
    help
    exit
    lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    �L$����q�U��W 2 26 681
    a.txt          2 27 6
    dir            1 28 48
    lsh> 
    ```

All operations performed as expected.

##### 2.2.3
###### Steps
1. Change shell to `lsh` and try `README | sh`, `ls | README` and `README | README`
2. Create `a.txt` with `ls` in it and try `cat a.txt | sh`
3. Create directory `dir`, create `a.txt` with `mkdir temp` in it inside `dir`, and try `cat dir/a.txt | sh`
4. Try `help | sh`

###### Expected results
1. Error messages printed
2. `ls` output by `sh` is displayed
3. `temp` directory created inside current directory
4. Help message printed and `sh` is not executed since `help` is a built-in command

###### Actual results
1. 
    ```shell
    SeaBIOS (version 1.16.3-debian-1.16.3-2)


    iPXE (https://ipxe.org) 00:03.0 CA00 PCI2.10 PnP PMM+1EFCAF60+1EF0AF60 CA00
                                                                                


    Booting from Hard Disk..xv6...
    cpu0: starting 0
    sb: size 2063 nblocks 1979 ninodes 200 nlog 30 logstart 2 inodestart 32 bmap start 83
    Scheduler default policy: DEFAULT
    init: starting lsh
    $ lsh
    lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    lsh> README | sh
    lsh: exec failed
    $ lsh> ls | README
    lsh: exec failed
    lsh> README | README
    lsh: exec failed
    lsh: exec failed
    lsh> 
    ```
2. 
    ```shell
    lsh> echo ls > a.txt
    lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    a.txt          2 26 3
    lsh> cat a.txt
    ls
    lsh> cat a.txt | sh
    $ .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    a.txt          2 26 3
    $ lsh> 
    ```
3. 
    ```shell
    $ lsh> mkdir dir
    lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    a.txt          2 26 3
    dir            1 27 32
    lsh> ls dir
    .              1 27 32
    ..             1 1 512
    lsh> echo mkdir temp > dir/a.txt
    lsh> ls dir
    .              1 27 48
    ..             1 1 512
    a.txt          2 28 11
    lsh> cat dir/a.txt
    mkdir temp
    lsh> cat dir/a.txt | sh
    $ $ lsh> ls
    .              1 1 512
    ..             1 1 512
    README         2 2 2290
    cat            2 3 18868
    echo           2 4 17648
    forktest       2 5 10008
    grep           2 6 22380
    init           2 7 18532
    kill           2 8 17676
    ln             2 9 17592
    ls             2 10 20544
    mkdir          2 11 17732
    rm             2 12 17716
    sh             2 13 34564
    stressfs       2 14 18660
    usertests      2 15 75732
    wc             2 16 19392
    zombie         2 17 17244
    head           2 18 22596
    cp             2 19 19536
    lsh            2 20 26628
    ps             2 21 17056
    nice           2 22 17044
    scheduler_test 2 23 17312
    sem_test       2 24 17080
    console        3 25 0
    a.txt          2 26 3
    dir            1 27 48
    temp           1 29 32
    lsh> 
    ```
4. 
    ```shell
    lsh> help | sh
    xv6 LSH
    Type program names and arguments, and hit enter.
    The following are built in:
    cd
    help
    exit
    lsh> 
    ```

All operations performed as expected.

#### 2.3
The default shell is changed to `lsh`.
```shell
SeaBIOS (version 1.16.3-debian-1.16.3-2)


iPXE (https://ipxe.org) 00:03.0 CA00 PCI2.10 PnP PMM+1EFCAF60+1EF0AF60 CA00
                                                                               


Booting from Hard Disk..xv6...
cpu0: starting 0
sb: size 2063 nblocks 1979 ninodes 200 nlog 30 logstart 2 inodestart 32 bmap start 83
Scheduler default policy: DEFAULT
init: starting lsh
lsh> 
```
