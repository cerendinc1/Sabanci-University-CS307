#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

int main() {
    int fd[2];
    pid_t pid_man, pid_grep;

    printf("I'm SHELL process, with PID: %d - Main command is: man diff | grep -A 2 -e -q\n",getpid());
    // Create a pipe
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(1);
    }
    
    // Fork the first child (man)
    pid_man = fork();
    if (pid_man < 0) {
        perror("fork");
        exit(1);
    }

    if (pid_man > 0) {
        pid_grep = fork();
        if (pid_grep < 0) {
            perror("fork");
            exit(1);
        }

        if (pid_grep == 0) {
            // Child process for grep
            printf("I'm GREP process, with PID: %d - My command is: grep -A 2 -e -q\n",getpid());

            // Redirect stdin from pipe
            dup2(fd[0], STDIN_FILENO);
            close(fd[1]); // Close unused write end
            close(fd[0]); // Close read end after dup2

            // Redirect stdout to file
            int file_fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (file_fd < 0) {
                perror("open");
                exit(1);
            }
            dup2(file_fd, STDOUT_FILENO);
            close(file_fd);

            // Execute grep command
            char *gargs[6];
            gargs[0] = strdup("grep");
            gargs[1] = strdup("-A");
            gargs[2] = strdup("2");
            gargs[3] = strdup("-e");
            gargs[4] = strdup("-q");
            gargs[5] = NULL;
            execvp(gargs[0], gargs);
            // If execvp returns, there was an error
            perror("execvp grep");
            exit(1);
        } 
        else {
            // Parent process, closing pipe fds since they are being used by children
            close(fd[0]);
            close(fd[1]);

            // Wait for the grep child process to finish
            wait(NULL);

            // Print completion message
            printf("I'm SHELL process, with PID: %d - execution is completed, you can find the results in output.txt\n", getpid());
        }
    }

    else {
        // Child process for man
        printf("I'm MAN process, with PID: %d - My command is: man diff\n",getpid());

        // Redirect stdout to pipe
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]); // Close unused read
        close(fd[1]); // Close write after dup2

        // Execute man command
        char *margs[3];
        margs[0] = strdup("man");
        margs[1] = strdup("diff");
        margs[2] = NULL;

        execvp(margs[0], margs);
        // If execvp returns, there was an error
        perror("execvp man");
        exit(1);
    } 
    

    return 0;
}
