// Ceren Dinc

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>


#define CMD_SIZE 1024
#define ARGS_SIZE 50

int proc_background[ARGS_SIZE]; // array to store background process IDs
int proc_background_count = 0; // counter for the number of background processes
pthread_mutex_t mtx; // mutex for thread synchronization


// functions that i use
void check_cmd(const char *command_tkn, char *const args[], const char *redirection_tkn, const char *file_name, char background_tkn);
void writeInput(FILE *parse_txt, const char *input_tkn);
void writeOption(FILE *parse_txt, const char *option_tkn);
void writeRedirection(FILE *parse_txt, const char *redirection_tkn);
void writeBackground(FILE *parse_txt, char background_tkn);
void writeCommand(FILE *parse_txt, const char *command_tkn);
void parse_command_line(char *line, char **command_tkn, char **input_tkn, char **option_tkn, char **redirection_tkn, char **file_name, char *background_tkn);
void *terminal_write(void *fd);

int main() {

    FILE *command_txt = fopen("command.txt", "r");
    char line[CMD_SIZE];
    pthread_mutex_init(&mtx, NULL);
   

    if (command_txt == NULL) {
        perror("Cannot open file");
        exit(1);
    }

    // read each line from the command file
    while (fgets(line, sizeof(line), command_txt)) {

        char *command_tkn = NULL;
        char *input_tkn = NULL;
        char *option_tkn = NULL;
        char *file_name = NULL;
        char background_tkn = 'n';
        char *redirection_tkn = "-";

        // extract tokens
        parse_command_line(line, &command_tkn, &input_tkn, &option_tkn, &redirection_tkn, &file_name, &background_tkn);

        FILE *parse_txt = fopen("parse.txt", "a");

        // write the parsed command data to the parse file
        fprintf(parse_txt, "----------\n");
        writeCommand(parse_txt, command_tkn);
        writeInput(parse_txt, input_tkn);
        writeOption(parse_txt, option_tkn);
        writeRedirection(parse_txt, redirection_tkn);
        writeBackground(parse_txt, background_tkn);
        fprintf(parse_txt, "----------\n");
        fclose(parse_txt);

        char *args[ARGS_SIZE] = {command_tkn, input_tkn, option_tkn, NULL}; 
        check_cmd(command_tkn, args, redirection_tkn, file_name, background_tkn);

    }

    // wait for all background processes to complete
    int counter = 0;

    while (counter < proc_background_count){
        waitpid(proc_background[counter], NULL, 0);
        counter = counter + 1;
    }

    pthread_mutex_destroy(&mtx); // destroy the mutex
    fclose(command_txt);
    return 0;
}

void writeInput(FILE *parse_txt, const char *input_tkn){
    // If input is NULL, write an empty string; otherwise, write the input.
    fprintf(parse_txt, "Inputs: %s\n", (input_tkn == NULL) ? "" : input_tkn);
}

void writeOption(FILE *parse_txt, const char *option_tkn) {
    // If option is NULL, write an empty string; otherwise, write the option.
    fprintf(parse_txt, "Options: %s\n", (option_tkn == NULL) ? "" : option_tkn);
}

void writeRedirection(FILE *parse_txt, const char *redirection_tkn) {
    fprintf(parse_txt, "Redirection: %s\n", redirection_tkn);
}

void writeBackground(FILE *parse_txt, char background_tkn) {
    fprintf(parse_txt, "Background Job: %c\n", background_tkn);
}

void writeCommand(FILE *parse_txt, const char *command_tkn) {
    fprintf(parse_txt, "Command: %s\n", command_tkn);
}


// parse the command line from a given line of text
void parse_command_line(char *line, char **command_tkn, char **input_tkn, char **option_tkn, char **redirection_tkn, char **file_name, char *background_tkn) {
    char *token;
    char token_line[CMD_SIZE];
    strcpy(token_line, line);

    token = strtok(token_line, " \n");
    if (token != NULL) {
        *command_tkn = strdup(token);
    }


    // Continue extracting tokens

    while (token != NULL) {
        token = strtok(NULL, " \n");

        if (token == NULL) break;

        if (token[0] == '>'){
            *redirection_tkn = strdup(token);
            token = strtok(NULL, " \n");
            if (token != NULL) {
                *file_name = strdup(token);
            }
        } 

        else if (token[0] == '&') {
            *background_tkn = 'y';
        }

        else if (token[0] == '-') {
            *option_tkn = strdup(token);
        } 

        else if (token[0] == '<'){
            *redirection_tkn = strdup(token);
            token = strtok(NULL, " \n");
            if (token != NULL) {
                *file_name = strdup(token);
            }
        }

        else {
            *input_tkn = strdup(token);
        }
    }
}

// execute the command based on the parsed tokens
void check_cmd(const char *command_tkn, char *const args[], const char *redirection_tkn, const char *file_name, char background_tkn) {

    int pid;
    int pipe_fd[2];
    pthread_t tid;

    // handle redirection cases
    if (strcmp(redirection_tkn, "-") != 0){
        pid = fork();

        if (pid > 0){
            // parent process --> handle background execution
            if (background_tkn == 'n') {
                waitpid(pid, NULL, 0);

            } 
            else {
                proc_background[proc_background_count++] = pid;
            }
        }

        else if (pid == 0) {
            // child process --> set up redirection
            if (strcmp(redirection_tkn, "<") == 0) {
                int fd;
                fd = open(file_name, O_RDONLY);
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            else if (strcmp(redirection_tkn, ">") == 0) {
                int fd;
                fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd, STDOUT_FILENO);
                close(fd);
            } 

            // execute the commadn
            execvp(command_tkn, args);
            perror("execvp failed");
            exit(EXIT_FAILURE);

        } 
    }

    else if (strcmp(redirection_tkn, "-") == 0){
        if (pipe(pipe_fd) != -1){
            if (pthread_create(&tid, NULL, terminal_write, (void *)&pipe_fd[0]) != 0){
                perror("Failed to create thread");
                exit(1);            
            }
            pid = fork();

            if (pid > 0){
                close(pipe_fd[1]);

                pthread_create(&tid, NULL, terminal_write, (void *) &pipe_fd[0]);
                
                if (pthread_create(&tid, NULL, terminal_write, (void *) &pipe_fd[0]) != 0) {
                    perror("Failed to create thread");
                    exit(1);
                }

                if (background_tkn == 'n') {
                    waitpid(pid, NULL, 0);
                    pthread_join(tid, NULL);
                } 
                else{
                    proc_background[proc_background_count++] = pid;
                }
                pthread_join(tid, NULL);
            }

            else if (pid == 0){
                close(pipe_fd[0]);
                dup2(pipe_fd[1], STDOUT_FILENO); 
                close(pipe_fd[1]);
                
                execvp(command_tkn, args);
                perror("execvp failed");
                exit(1);
            }
        }
        else{
            perror("pipe failed");
            exit(1);
        }
    }
}

// handle writing output from a file descriptor in a separate thread

void *terminal_write(void *fd) {
    int pipe_fd = *((int *)fd);
    FILE *stream;
    char line[CMD_SIZE];
    long thread_id = pthread_self();

    // Open the stream from the file descriptor
    stream = fdopen(pipe_fd, "r");
    if (stream == NULL) {
        perror("fdopen failed");
        close(pipe_fd);
        return NULL;
    }

    // Lock the mutex before writing to the terminal
    pthread_mutex_lock(&mtx);
    printf("---- %ld\n", thread_id);

    // Read and print lines from the stream
    while (fgets(line, CMD_SIZE, stream)) {
        printf("%s", line);
    }

    // Unlock the mutex after writing to the terminal
    printf("---- %ld\n", thread_id);
    pthread_mutex_unlock(&mtx);

    // Clean up
    fflush(stdout);
    fclose(stream);
    close(pipe_fd);

    return NULL;
}
