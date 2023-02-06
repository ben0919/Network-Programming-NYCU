#include "def.h"
#include "utils.h"
#include "execute.h"
#include "command.h"
#include "npshell.h"


void np_shell() {
    string line;
    vector<string> input_argv;
    map<unsigned int, int*> numbered_pipes;
    unsigned int line_count = 0;
    env_variables_init();
    while(true) {
        show_command_line_prompt();
        line = read_line();
        line = delete_special_character(line);
        input_argv = split(line, " ");
        if (do_build_in_command(input_argv) != 0) {
            continue;
        }
        vector<command*> commands = parse(input_argv);
        line_count = execute_commands(commands, numbered_pipes, line_count);
        delete_commands(commands);
    }
}