
/*
With this program, user type texts line by line, where each line has a maximum length of 127 characters.  
Each line is considered as "entered" (i.e., finalized) when the ENTER or RETURN key is clicked.
The program counts the number of characters entered for each minute.
When user types ":exit", the program exits.  
The Makefile should be changed to include this program and thus it can be run from the shell of xv6.
*/

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include <stdbool.h>

#define MAX_LINE_SIZE 128
#define MAX_BUF_SIZE 1280
char buf[MAX_BUF_SIZE];

void panic(char *s)
{
    fprintf(2, "%s\n", s);
    exit(1);
}

//create a new process
int fork1(void)
{
    int pid;
    pid = fork();
    if (pid == -1)
        panic("fork");
    return pid;
}

//create a pipe
void pipe1(int fd[2])
{
    int rc = pipe(fd);
    if (rc < 0)
    {
        panic("Fail to create a pipe.");
    }
}

//pull everything from pipe and return the size of the content
int read1(int *fd)
{
    char ch = 'a';
    write(fd[1], &ch, 1); //write something to the pipe so the next read will not block (because read operation is a blocking operation - if the pipe is empty, read will wait till something is put in)
    int len = read(fd[0], buf, MAX_BUF_SIZE) - 1;
    return len;
}

// Helper to validate an argument is a valid integer
bool isNumber(char number[])
{
    int i = 0;

    //checking for negative numbers
    if (number[0] == '-')
        i = 1;
    for (; number[i] != 0; i++)
    {
        if (number[i] > '9' || number[i] < '0')
            return false;
    }
    return true;
}

int main(int argc, char *argv[])
{

    // We expect three arguments, always.
    // 1st is always there for running the program
    // 2nd is the runningTime
    // 3rd is the intervalTime
    if (argc != 3)
    {
        printf("Insufficient arguments. Provide Running Time & Interval Time\n");
        exit(1);
    }

    // Use the helper function to validate a user
    // enters an integer and not any other characters
    if (!isNumber(argv[1]) || !isNumber(argv[2]))
    {
        printf("Enter valid integers\n");
        exit(1);
    }

    int runningTime = atoi(argv[1]);
    int intervalTime = atoi(argv[2]);

    if (runningTime % intervalTime != 0)
    {
        printf("Make sure Running Time is a multiple of the Interval Time!\n");
        exit(1);
    }

    //create two pipe to share with child
    int fd1[2], fd2[2];
    pipe1(fd1); //a pipe from child to parent - child sends entered texts to parent for counting
    pipe1(fd2); //a pipe from child to parent - child lets parent stop (when user types :exit)

    int result = fork1(); //create child process
    if (result == 0)
    {
        // child process:

        close(fd1[0]);
        close(fd2[0]);

        while (runningTime > 0)
        {
            const char robotInput[] = "Hello!";

            // sleep() for `intervalTime` seconds then subtract from runningTime to
            // decrease time remaining
            sleep(intervalTime);
            runningTime -= intervalTime;

            // Write 'Hello!' to the Parent so it can count the
            // characters every minute
            write(fd1[1], robotInput, strlen(robotInput));

            // Child should still print 'Hello!' to be seen in
            // the console.
            printf("%s\n", robotInput);
        }

        // Outside of the while loop means running time has been reached.
        // Exit the child process and inform the parent to exit by writing to Pipe2.
        char ch = 'a';
        write(fd2[1], &ch, 1);
        exit(0);
    }
    else
    {
        //parent process:

        while (1)
        {
            sleep(60);

            int len = read1(fd1);
            printf("\nIn last minute, %d characters were entered.\n", len);

            len = read1(fd2);
            if (len > 0)
            {
                //now to terminate
                printf("Times up!\n");
                wait(0);
                exit(0);
            }
        }
    }
}
