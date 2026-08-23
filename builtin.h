#ifndef BUILTIN_H
#define BUILTIN_H

#include "parser.h"

/*
 * The parser already defines Command.
 * The builtin/executor code supplied for this milestone
 * uses the name command_t.
 */
typedef Command command_t;

/* Check whether a command is a builtin */
int is_builtin(const command_t *cmd);

/* Execute a builtin command */
int execute_builtin(command_t *cmd);

#endif
