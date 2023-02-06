#include "def.h"
#include "utils.h"

void env_variables_init() {
    setenv("PATH", "bin:.", 1);
}

void show_command_line_prompt() {
    cout << "% ";
}

string read_line() {
    string line;
    getline(cin, line);
    if (!cin) {
        exit(0);
    }
    return line;
}

const vector<string> split(const string &s, const string &pattern){
    vector<string> result;
    string::size_type begin, end;
    begin = 0;
    while((end = s.find_first_of(pattern, begin)) != string::npos) {
        if (end - begin > 0) {
            result.push_back(s.substr(begin, end - begin));
        }
        begin = end + pattern.length();
    }
    if(begin < s.length()) {
        result.push_back(s.substr(begin));
    }
    return result;
}

vector<command*> parse(vector<string> &input_argv) {
    vector<command*> commands;
    vector<string> argv;
    bool pipe_before = false;
    bool numbered_pipe_before = false;
    int N = input_argv.size();
    for(int i = 0; i < N; ++i) {
        if (input_argv[i].find("|") != string::npos) {
            if (input_argv[i] == "|") {
                command *temp = new command(PIPE, argv, pipe_before, true, numbered_pipe_before, false, false);
                commands.push_back(temp);
                pipe_before = true;
                numbered_pipe_before = false;
                argv.clear();
            } else {
                int pipe_number = stoi(input_argv[i].substr(1));
                command *temp = new command(NUMBERED_PIPE, argv, pipe_number, pipe_before, false, numbered_pipe_before, true, false);
                commands.push_back(temp);
                pipe_before = false;
                numbered_pipe_before = true;
                argv.clear();
            }
        } else if (input_argv[i].find("!") != string::npos) {
            int pipe_number = stoi(input_argv[i].substr(1));
            command *temp = new command(NUMBERED_PIPE, argv, pipe_number, pipe_before, false, numbered_pipe_before, true, true);
            commands.push_back(temp);
            pipe_before = false;
            numbered_pipe_before = true;
            argv.clear();
        } else if (input_argv[i].find(">") != string::npos) {
            if (i == input_argv.size() - 1) {
                cout << "Usage: [command] > [file_name] " << endl;
                break;
            } else {
                command *temp = new command(REDIRECT, argv, input_argv[i+1], pipe_before, false, numbered_pipe_before, false, false);
                commands.push_back(temp);
                break;
            }
        } else if (i == N-1) {
            argv.push_back(input_argv[i]);
            command *temp = new command(PIPE, argv, pipe_before, false, numbered_pipe_before, false, false);
            commands.push_back(temp);
        } else {
            argv.push_back(input_argv[i]);
        }
        
    }
    return commands;
}

void delete_commands(vector<command*> &commands) {
    for (int i = 0; i < commands.size(); ++i) {
        delete commands[i];
    }
}