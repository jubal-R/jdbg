#include "debugger.h"
#include "breakpoint.h"

#include <string>
#include <iostream>
#include <sstream>
#include <iterator>
#include <regex>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>


void Debugger::setupInferior(const char *path) {
    ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
//    execl(path, path, nullptr);
    execl("testbin", "testbin", nullptr);
}

void Debugger::debug() {
    while(1) {
        int status;
        waitpid(pid, &status, 0);

        if (WIFSTOPPED(status)) {
            switch (WSTOPSIG(status)) {
                case SIGTRAP:
                    std::cout << "Stopped on SIGTRAP" << std::endl;
                    break;
                case SIGILL:
                    std::cout << "Stopped on SIGILL" << std::endl;
                    break;
                case SIGSEGV:
                    std::cout << "Stopped on SIGSEGV" << std::endl;
                    break;
                default:
                    break;
            }

            prompt();
        } else if (WIFEXITED(status)) {
            std::cout << "Target terminated - exiting" << std::endl;
            exit(0);
        } else if (WIFSIGNALED(status)) {
            std::cout << "Process terminated by signal" << std::endl;
            exit(0);
        }
    }
}

std::string Debugger::getCommand() {
    char* line = nullptr;
    line = readline("jdbg> ");
    if (strlen(line) > 0) {
        add_history(line);
    }

    std::string command(line);
    free(line);

    return command;
}

void Debugger::prompt() {
    while(1) {
        const std::string input = getCommand();
        const auto tokens = getTokens(input);

        if (tokens.size() > 0){
            if (tokens[0] == "c") {
                handleContinue();
                break;
            } else if (tokens[0] == "b"){
                handleBreakpoint(tokens);
            } else {
                std::cout << "Invalid Command" << std::endl;
            }
        }
    }
}

void Debugger::handleContinue() {
    std::cout << "Continuing..." << std::endl;
    checkForBreakpoint();
    ptrace(PTRACE_CONT, pid, nullptr, 0);
}

void Debugger::handleBreakpoint(std::vector<std::string> tokens) {
    if (tokens.size() > 1) {
        if (std::regex_match(tokens[1], std::regex("0[xX][0-9a-fA-F]{1,16}"))) {
            auto addr = stol(tokens[1], 0, 16);
            setBreakpoint(addr);
        } else {
            std::cout << "Invalid address. Example: b 0x402a00" << std::endl;
        }
    } else {
        std::cout << "Specify address. Example: b 0x402a00" << std::endl;
    }
}

// Split string into tokens using space as delimeter
std::vector<std::string> Debugger::getTokens(std::string input) {
    std::istringstream inputStream{input};
    std::vector<std::string> tokens{std::istream_iterator<std::string>{inputStream}, std::istream_iterator<std::string>{}};
    return tokens;
}

void Debugger::setBreakpoint(const std::uintptr_t address) {
    Breakpoint breakpoint{pid, address};
    breakpoint.enable();
    breakpointsMap[address] = breakpoint;
    std::cout << "Breakpoint set at 0x" << std::hex << address << std::endl;
}

void Debugger::checkForBreakpoint() {
    user_regs_struct regs = getRegisters();
    auto search = breakpointsMap.find(regs.rip-1);
    if (search != breakpointsMap.end()){
        breakpointsMap[regs.rip-1].disable();
        regs.rip--;
        ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
    }
}

user_regs_struct Debugger::getRegisters() {
    user_regs_struct registers;
    ptrace(PTRACE_GETREGS, pid, nullptr, &registers);

    return registers;
}
