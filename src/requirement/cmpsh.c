#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

// Global variables
char *path[100]; // Store directories in the PATH variable
int path_count = 0;

// Function declarations
void handle_signal(int sig);
void execute_command(char *input);
void execute_pipeline(char *input);
int cd_command(char *dir);
int pwd_command();
int path_command(char *input);

int main(int argc, char **argv) {
    char *input = NULL;
    size_t len = 0;
    ssize_t read;

    // Signal handling setup
    signal(SIGINT, handle_signal);
    signal(SIGTSTP, handle_signal);

    while (1) {
        printf("cmpsh> ");
        read = getline(&input, &len, stdin);
        if (read == -1) {
            break;
        }

        input[strcspn(input, "\n")] = 0; // Remove newline

        if (strcmp(input, "exit") == 0) {
            break;
        }

        if (strncmp(input, "cd", 2) == 0) {
            cd_command(input + 3);
            continue;
        }

        if (strcmp(input, "pwd") == 0) {
            pwd_command();
            continue;
        }

        if (strncmp(input, "path", 5) == 0) {
            path_command(input);
            continue;
        }

        // Check for pipes
        if (strchr(input, '|')) {
            execute_pipeline(input);
        } else {
            execute_command(input);
        }
    }

    free(input);
    return 0;
}

void handle_signal(int sig) {
    printf("\nCaught signal %d. Type 'exit' to quit.\n", sig);
}

int cd_command(char *dir) {
    if (chdir(dir) != 0) {
        perror("cd failed");
        return -1;
    }
    return 0;
}

int pwd_command() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd failed");
    }
    return 0;
}

int path_command(char *input) {
    // Clear the current path list
    path_count = 0;
    char *token = strtok(input + 6, " "); // Skip the 'path' keyword
    while (token != NULL) {
        path[path_count++] = token;
        token = strtok(NULL, " ");
    }
    if (path_count == 0) {
        printf("Path are cleared.\n");
    } else {
        printf("Path updated.\n");
    }
    return 0;
}

void execute_command(char *input) {
    pid_t pid;
    int status;
    char *args[100];
    char *token;
    int i = 0;
    int out_redirect = 0, in_redirect = 0;
    char *out_file = NULL, *in_file = NULL;

    token = strtok(input, " ");
    while (token != NULL) {
        if (strcmp(token, ">") == 0) {
            out_redirect = 1;
            token = strtok(NULL, " ");
            out_file = token;
        } else if (strcmp(token, "<") == 0) {
            in_redirect = 1;
            token = strtok(NULL, " ");
            in_file = token;
        } else {
            args[i++] = token;
        }
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    pid = fork();
    if (pid == -1) {
        perror("Fork failed");
        return;
    }

    if (pid == 0) {
        if (out_redirect && out_file) {
            int fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("Failed to open output file");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if (in_redirect && in_file) {
            int fd = open(in_file, O_RDONLY);
            if (fd == -1) {
                perror("Failed to open input file");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (execvp(args[0], args) == -1) {
            perror("Execution failed");
        }
        exit(1);
    } else {
        waitpid(pid, &status, 0);
    }
}

void execute_pipeline(char *input) {
    char *commands[10];
    int num_cmds = 0;
    char *token = strtok(input, "|");
    
    while (token != NULL) {
        commands[num_cmds++] = token;
        token = strtok(NULL, "|");
    }

    int pipe_fds[2 * (num_cmds - 1)];
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipe_fds + i * 2) < 0) {
            perror("Pipe failed");
            exit(1);
        }
    }

    for (int i = 0; i < num_cmds; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            if (i > 0) {
                dup2(pipe_fds[(i - 1) * 2], STDIN_FILENO);
            }
            if (i < num_cmds - 1) {
                dup2(pipe_fds[i * 2 + 1], STDOUT_FILENO);
            }
            for (int j = 0; j < 2 * (num_cmds - 1); j++) {
                close(pipe_fds[j]);
            }

            char *args[100];
            int k = 0;
            token = strtok(commands[i], " ");
            while (token != NULL) {
                args[k++] = token;
                token = strtok(NULL, " ");
            }
            args[k] = NULL;

            execvp(args[0], args);
            perror("Execution failed");
            exit(1);
        }
    }

    for (int i = 0; i < 2 * (num_cmds - 1); i++) {
        close(pipe_fds[i]);
    }

    for (int i = 0; i < num_cmds; i++) {
        wait(NULL);
    }
}