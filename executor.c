#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "executor.h"
#include "builtin.h"


/*
 * Execute a single command.
 *
 * Examples:
 *
 *     ls
 *     pwd
 *     echo hello
 */
int execute_command(command_t *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
    {
        return -1;
    }

    /*
     * Built-in commands are executed by the shell.
     */
    if (is_builtin(cmd))
    {
        return execute_builtin(cmd);
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return -1;
    }

    /*
     * CHILD
     */
    if (pid == 0)
    {
        /*
         * Input redirection
         */
        if (cmd->input != NULL)
        {
            int fd = open(cmd->input, O_RDONLY);

            if (fd < 0)
            {
                perror("open input");
                exit(EXIT_FAILURE);
            }

            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        /*
         * Output redirection
         */
        if (cmd->output != NULL)
        {
            int flags = O_WRONLY | O_CREAT;

            if (cmd->append)
                flags |= O_APPEND;
            else
                flags |= O_TRUNC;

            int fd = open(cmd->output, flags, 0644);

            if (fd < 0)
            {
                perror("open output");
                exit(EXIT_FAILURE);
            }

            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        /*
         * Execute external command.
         *
         * cmd->argv is already NULL terminated
         * by the parser.
         */
        execvp(cmd->argv[0], cmd->argv);

        /*
         * execvp only returns if it failed.
         */
        perror("Shellforge");
        exit(EXIT_FAILURE);
    }

    /*
     * PARENT
     */
    int status;

    if (waitpid(pid, &status, 0) == -1)
    {
        perror("waitpid");
        return -1;
    }

    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }

    return -1;
}


/*
 * ============================================================
 * EXECUTE PIPELINE
 * ============================================================
 *
 * Example:
 *
 *     echo hello | wc
 *
 *             PIPE
 *     echo -------------> wc
 *     stdout             stdin
 *
 * ============================================================
 */
int execute_pipeline(Pipeline *p)
{
    if (p == NULL || p->count == 0)
    {
        return -1;
    }

    /*
     * If there is only one command,
     * there is no pipe required.
     */
    if (p->count == 1)
    {
        return execute_command(&p->commands[0]);
    }

    pid_t pids[MAX_COMMANDS];

    /*
     * File descriptor for the read end of
     * the previous pipe.
     *
     * -1 means there is no previous pipe.
     */
    int previous_fd = -1;

    /*
     * Create every command.
     */
    for (int i = 0; i < p->count; i++)
    {
        int pipefd[2];

        /*
         * Every command except the last one
         * needs a pipe.
         */
        if (i < p->count - 1)
        {
            if (pipe(pipefd) == -1)
            {
                perror("pipe");
                return -1;
            }
        }

        /*
         * Create child.
         */
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("fork");
            return -1;
        }

        /*
         * ====================================================
         * CHILD
         * ====================================================
         */
        if (pid == 0)
        {
            command_t *cmd = &p->commands[i];

            /*
             * --------------------------------------------
             * INPUT FROM PREVIOUS PIPE
             * --------------------------------------------
             *
             * For:
             *
             *     echo hello | wc
             *
             * wc gets its stdin from the pipe.
             */
            if (previous_fd != -1)
            {
                if (dup2(previous_fd, STDIN_FILENO) == -1)
                {
                    perror("dup2 stdin");
                    exit(EXIT_FAILURE);
                }

                close(previous_fd);
            }

            /*
             * --------------------------------------------
             * OUTPUT TO NEXT PIPE
             * --------------------------------------------
             *
             * echo sends stdout into the pipe.
             */
            if (i < p->count - 1)
            {
                close(pipefd[0]);

                if (dup2(pipefd[1], STDOUT_FILENO) == -1)
                {
                    perror("dup2 stdout");
                    exit(EXIT_FAILURE);
                }

                close(pipefd[1]);
            }

            /*
             * --------------------------------------------
             * INPUT REDIRECTION
             * --------------------------------------------
             */
            if (cmd->input != NULL)
            {
                int fd = open(cmd->input, O_RDONLY);

                if (fd < 0)
                {
                    perror("open input");
                    exit(EXIT_FAILURE);
                }

                if (dup2(fd, STDIN_FILENO) == -1)
                {
                    perror("dup2 input file");
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                close(fd);
            }

            /*
             * --------------------------------------------
             * OUTPUT REDIRECTION
             * --------------------------------------------
             */
            if (cmd->output != NULL)
            {
                int flags = O_WRONLY | O_CREAT;

                if (cmd->append)
                    flags |= O_APPEND;
                else
                    flags |= O_TRUNC;

                int fd = open(cmd->output, flags, 0644);

                if (fd < 0)
                {
                    perror("open output");
                    exit(EXIT_FAILURE);
                }

                if (dup2(fd, STDOUT_FILENO) == -1)
                {
                    perror("dup2 output file");
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                close(fd);
            }

            /*
             * --------------------------------------------
             * EXECUTE COMMAND
             * --------------------------------------------
             *
             * echo is also available as an external
             * command (/usr/bin/echo), so this works
             * correctly inside the pipeline.
             */
            execvp(cmd->argv[0], cmd->argv);

            /*
             * Only reached if execvp fails.
             */
            perror("Shellforge");
            exit(EXIT_FAILURE);
        }

        /*
         * ====================================================
         * PARENT
         * ====================================================
         */

        pids[i] = pid;

        /*
         * Parent no longer needs the previous pipe.
         */
        if (previous_fd != -1)
        {
            close(previous_fd);
        }

        /*
         * Keep the read end of the current pipe.
         *
         * The next command will use it as stdin.
         */
        if (i < p->count - 1)
        {
            close(pipefd[1]);
            previous_fd = pipefd[0];
        }
        else
        {
            previous_fd = -1;
        }
    }

    /*
     * Wait for every command in the pipeline.
     */
    int final_status = 0;

    for (int i = 0; i < p->count; i++)
    {
        int status;

        if (waitpid(pids[i], &status, 0) == -1)
        {
            perror("waitpid");
            final_status = -1;
            continue;
        }

        /*
         * Return the status of the last command.
         */
        if (i == p->count - 1)
        {
            if (WIFEXITED(status))
            {
                final_status = WEXITSTATUS(status);
            }
            else
            {
                final_status = -1;
            }
        }
    }

    return final_status;
}
