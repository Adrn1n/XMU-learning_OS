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

### 2
#### 2.1

#### 2.2
##### 2.2.1

##### 2.2.2

##### 2.2.3

#### 2.3

## Results
### 1

### 2
#### 2.1

#### 2.2
##### 2.2.1

##### 2.2.2

##### 2.2.3

#### 2.3
