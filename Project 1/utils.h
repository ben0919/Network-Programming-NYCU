#ifndef UTILS_H
#define UTILS_H

#include "def.h"
#include "command.h"

string read_line();
void env_variables_init();
void show_command_line_prompt();
const vector<string> split(const string &s, const string &pattern);
vector<command*> parse(vector<string> &input_argv);
void delete_commands(vector<command*> &commands);
string home_dir();
void save_command_line(string line);
#endif