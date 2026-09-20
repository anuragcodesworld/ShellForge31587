#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "builtin.h"

int execute_command(command_t *cmd);
int execute_pipeline(Pipeline *p);

#endif
