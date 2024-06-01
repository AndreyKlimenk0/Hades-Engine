#ifndef COMMANDS_H
#define COMMANDS_H

#include "../libs/str.h"
#include "../libs/ds/array.h"

void init_commands();
void run_command(const char *command_name, Array<String> &command_args);

#endif