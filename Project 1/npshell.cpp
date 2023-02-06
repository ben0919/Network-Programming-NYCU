#include "def.h"
#include "utils.h"
#include "execute.h"
#include "command.h"

string line;
vector<string> input_argv;
map<unsigned int, int*> numbered_pipes;
unsigned int line_count = 0;

int main() {
    env_variables_init();
    while(true) {
        show_command_line_prompt();
        line = read_line();
        input_argv = split(line, " ");
        if (do_build_in_command(input_argv) != 0) {
            continue;
        }
        vector<command*> commands = parse(input_argv);
        execute_commands(commands);
        delete_commands(commands);
    }
}