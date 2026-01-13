#ifndef COMMANDS_H
#define COMMANDS_H

#include "../libs/str.h"
#include "../libs/structures/array.h"

struct Engine;

void init_commands(Engine *_engine);
void run_command(const char *command_name, Array<String> &command_args);

#endif
