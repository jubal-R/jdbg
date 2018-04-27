#include "debugger.h"

#include <iostream>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>

Debugger::Debugger(pid_t p)
{
    pid = p;
}

void Debugger::setupInferior(const char *path) {
    ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
//    execl(path, path, nullptr);
    execl("/bin/ls", "ls", nullptr);
}

void Debugger::debug() {
    while(1) {
        int status;
        waitpid(pid, &status, 0);

        if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
            std::cout << "Stopped on SIGTRAP" << std::endl;
            prompt();

        } else if (WIFEXITED(status)) {
            std::cout << "Target terminated - exiting" << std::endl;
            exit(0);
        }
    }
}

std::string Debugger::getCommand() {
    char* line;
    line = readline("dbg> ");
        if (strlen(line) > 0) {
            add_history(line);
        }

        std::string command(line);
        free(line);

        return command;
}

void Debugger::prompt() {
    while(1) {
        std::string input = getCommand();

        if (input == "c") {
            std::cout << "Continuing..." << std::endl;
            ptrace(PTRACE_CONT, pid, nullptr, 0);
            break;

        } else {
            std::cout << "Invalid Command" << std::endl;
        }
    }
}
