# Alex Ong

## Assignment 2

### Files Changed

Working with xv6 to add system calls to the on both the kernel and user side.

-   [ ] getppid(): Returns current process' parent id
-   [ ] getcpids(): Returns number of child processes a process has modifies an array to contain the pid of each child
-   [ ] getswapcount(): Return number of CPU swaps (context switches) that the current process has gone through.

To accomplish adding these three system calls, multiple files on the kernel and user side had to be modified.

Kernel:

-   [ ] syscall.h: add system call number
-   [ ] syscall.c: add prototypes
-   [ ] proc.h: add swapcount field to proc struct
-   [ ] proc.c: implement the three system call functions

User:

-   usys.pl: add the three system calls for building assembly
-   [ ] user.h: add system declarations for User to call

Modified Makefile to be able to run test file, `testsyscall.c` from the shell to test these three system calls.

# Assignment 3

Adding a Linux CFS scheduler to our xv6 program

## Files Changed

-   proc.c: Main implementation of the cfs scheduler is done here as well as implementing new system calls.
-   proc.h: added nice and vruntime fields to the proc struct
-   syscall.c: define three new system calls for the cfs scheduler:
    -   nice(): to update nice value
    -   startcfs(): start the cfs scheduler
    -   stopcfs(): stop the cfs scheduler
-   syscall.h: define the three new system calls above
-   user.h: define the three new system calls
-   usys.pl: define the three new system calls
-   testsyscall.c: Test file to test our cfs scheduler, output below

### Test 3.4 Output

Start CFS
process (pid=3) has nice = 0
process (pid=4) has nice = 10
process (pid=5) has nice = 10
process (pid=6) has nice = 10
process (pid=7) has nice = 10
process (pid=8) has nice = 10
process (pid=9) has nice = 10
process (pid=10) has nice = 10
process (pid=11) has nice = 10
process (pid=12) has nice = 10
process (pid=13) has nice = 10
[DEBUG CFS] Process 3 will run for 10 timeslices next!
[DEBUG CFS] Process 3 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 4 will run for 6 timeslices next!
[DEBUG CFS] Process 4 used up 6 of its assigned 6 timeslices and is swapped out !
[DEBUG CFS] Process 5 will run for 6 timeslices next!
[DEBUG CFS] Process 5 used up 6 of its assigned 6 timeslices and is swapped out !
[DEBUG CFS] Process 6 will run for 6 timeslices next!
[DEBUG CFS] Process 6 used up 6 of its assigned 6 timeslices and is swapped out !
[DEBUG CFS] Process 7 will run for 6 timeslices next!
[DEBUG CFS] Process 7 used up 6 of its assigned 6 timeslices and is swapped out !
[DEBUG CFS] Process 8 will run for 6 timeslices next!
[DEBUG CFS] Process 8 used up 6 of its assigned 6 timeslices and is swapped out !
[DEBUG CFS] Process 9 will run for 6 timeslices next!
[DEBUG CFS] Process 9 used up 6 of its assigned 6 timeslices and is swapped out !
[DEBUG CFS] Process 10 will run for 6 timeslices next!
[DEBUG CFS] Process 10 used up 6 of its assigned 6 timeslices and is swapped out !
[DEBUG CFS] Process 11 will run for 6 timeslices next!
[DEBUG CFS] Process 11 used up 6 of its assigned 6 timeslices and is swapped out !
[DEBUG CFS] Process 12 will run for 6 timeslices next!
[DEBUG CFS] Process 12 used up 6 of its assigned 6 timeslices and is swapped out !
[DEBUG CFS] Process 13 will run for 6 timeslices next!
[DEBUG CFS] Process 13 used up 6 of its assigned 6 timeslices and is swapped out !
[DEBUG CFS] Process 3 will run for 10 timeslices next!
[DEBUG CFS] Process 3 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 3 will run for 10 timeslices next!
[DEBUG CFS] Process 3 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 3 will run for 10 timeslices next!
[DEBUG CFS] Process 3 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 3 will run for 10 timeslices next!
[DEBUG CFS] Process 3 used up 1 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 4 will run for 10 timeslices next!
[DEBUG CFS] Process 4 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 5 will run for 10 timeslices next!
[DEBUG CFS] Process 5 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 6 will run for 10 timeslices next!
[DEBUG CFS] Process 6 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 7 will run for 10 timeslices next!
[DEBUG CFS] Process 7 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 8 will run for 10 timeslices next!
[DEBUG CFS] Process 8 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 9 will run for 10 timeslices next!
[DEBUG CFS] Process 9 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 10 will run for 10 timeslices next!
[DEBUG CFS] Process 10 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 11 will run for 10 timeslices next!
[DEBUG CFS] Process 11 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 12 will run for 10 timeslices next!
[DEBUG CFS] Process 12 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 13 will run for 10 timeslices next!
[DEBUG CFS] Process 13 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 4 will run for 10 timeslices next!
[DEBUG CFS] Process 4 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 5 will run for 10 timeslices next!
[DEBUG CFS] Process 5 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 6 will run for 10 timeslices next!
[DEBUG CFS] Process 6 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 7 will run for 10 timeslices next!
[DEBUG CFS] Process 7 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 8 will run for 10 timeslices next!
[DEBUG CFS] Process 8 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 9 will run for 10 timeslices next!
[DEBUG CFS] Process 9 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 10 will run for 10 timeslices next!
[DEBUG CFS] Process 10 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 11 will run for 10 timeslices next!
[DEBUG CFS] Process 11 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 12 will run for 10 timeslices next!
[DEBUG CFS] Process 12 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 13 will run for 10 timeslices next!
[DEBUG CFS] Process 13 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 4 will run for 10 timeslices next!
[DEBUG CFS] Process 4 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 5 will run for 10 timeslices next!
[DEBUG CFS] Process 5 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 6 will run for 10 timeslices next!
[DEBUG CFS] Process 6 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 7 will run for 10 timeslices next!
[DEBUG CFS] Process 7 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 8 will run for 10 timeslices next!
[DEBUG CFS] Process 8 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 9 will run for 10 timeslices next!
[DEBUG CFS] Process 9 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 10 will run for 10 timeslices next!
[DEBUG CFS] Process 10 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 11 will run for 10 timeslices next!
[DEBUG CFS] Process 11 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 12 will run for 10 timeslices next!
[DEBUG CFS] Process 12 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 13 will run for 10 timeslices next!
[DEBUG CFS] Process 13 used up 10 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 4 will run for 10 timeslices next!
[DEBUG CFS] Process 4 used up 1 of its assigned 10 timeslices and is swapped out !
[DEBUG CFS] Process 3 will run for 10 timeslices next!
Stopped CFS
