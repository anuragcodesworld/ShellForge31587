#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "executor.h"
#include "builtin.h"


/*
 * Execute one command.
 *
 * Used when there is no pipe.
 *
 * Example:
 *
 *     ls
 *     pwd
 *     echo hello
 */
int execute_command(command_t *cmd)
{
    pid_t pid;
    int status;

    if (cmd == NULL || cmd->argc == 0)
    {
        return -1;
    }

    /*
     * Builtins run directly in the shell.
     *
     * This is important for commands such as cd.
     */
    if (is_builtin(cmd))
    {
        return execute_builtin(cmd);
    }

    pid = fork();

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
        execvp(cmd->argv[0], cmd->argv);

        perror("Shellforge");
        exit(EXIT_FAILURE);
    }

    /*
     * PARENT
     */
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
 * Execute an entire pipeline
 * ============================================================
 *
 * Example:
 *
 *     echo hello | wc
 *
 * becomes:
 *
 *
 *     echo hello
 *          |
 *          | pipe
 *          v
 *          wc
 *
 *
 * echo's stdout is connected to wc's stdin.
 */
int execute_pipeline(Pipeline *pipeline)
{
    if (pipeline == NULL || pipeline->count == 0)
    {
        return -1;
    }

    /*
     * No pipe needed for a single command.
     */
    if (pipeline->count == 1)
    {
        return execute_command(&pipeline->commands[0]);
    }

    pid_t pids[MAX_COMMANDS];

    /*
     * Read end of the previous pipe.
     *
     * -1 means there is no previous pipe.
     */
    int previous_fd = -1;

    /*
     * Create each command in the pipeline.
     */
    for (int i = 0; i < pipeline->count; i++)
    {
        int pipefd[2];

        /*
         * Every command except the last one
         * needs a pipe.
         */
        if (i < pipeline->count - 1)
        {
            if (pipe(pipefd) == -1)
            {
                perror("pipe");
                return -1;
            }
        }

        /*
         * Create child process.
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
            command_t *cmd = &pipeline->commands[i];

            /*
             * ------------------------------------------------
             * Connect previous pipe to stdin.
             * ------------------------------------------------
             *
             * This applies to every command except
             * the first command.
             *
             * Example:
             *
             * echo hello | wc
             *
             * wc gets:
             *
             * stdin <- pipe
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
             * ------------------------------------------------
             * Connect stdout to the next pipe.
             * ------------------------------------------------
             *
             * This applies to every command except
             * the last command.
             *
             * Example:
             *
             * echo hello | wc
             *
             * echo gets:
             *
             * stdout -> pipe
             */
            if (i < pipeline->count - 1)
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
             * Execute the command.
             *
             * We intentionally use execvp() here even for
             * commands such as echo when they are inside a
             * pipeline.
             */
            execvp(cmd->argv[0], cmd->argv);

            /*
             * execvp() only returns if execution failed.
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
         * The next command will use this as stdin.
         */
        if (i < pipeline->count - 1)
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
     * Wait for all commands.
     */
    int final_status = 0;

    for (int i = 0; i < pipeline->count; i++)
    {
        int status;

        if (waitpid(pids[i], &status, 0) == -1)
        {
            perror("waitpid");
            final_status = -1;
            continue;
        }

        /*
         * Return the status of the final command.
         */
        if (i == pipeline->count - 1)
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
