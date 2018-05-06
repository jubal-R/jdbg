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
        onBreakpoint = false;
        int status;
        waitpid(pid, &status, 0);

        if (WIFSTOPPED(status)) {
            switch (WSTOPSIG(status)) {
                case SIGTRAP:
                    handleSIGTRAP();
                    break;
                case SIGILL:
                    std::cout << "Stopped on SIGILL\n";
                    break;
                case SIGSEGV:
                    std::cout << "Stopped on SIGSEGV\n";
                    break;
                default:
                    break;
            }

            prompt();
        } else if (WIFEXITED(status)) {
            std::cout << "Target terminated - exiting\n";
            exit(0);
        } else if (WIFSIGNALED(status)) {
            std::cout << "Process terminated by signal\n";
            exit(0);
        }
    }
}

siginfo_t Debugger::getSigInfo() {
    siginfo_t sigInfo;
    ptrace(PTRACE_GETSIGINFO, pid, nullptr, &sigInfo);
    return sigInfo;
}

void Debugger::handleSIGTRAP() {
    siginfo_t sigInfo = getSigInfo();
    switch (sigInfo.si_code) {
    // Breakpoint
    case TRAP_BRKPT:
    case SI_KERNEL:
        onBreakpoint = true;
        std::cout << "Stopped on breakpoint\n";
        break;
    // Single Step
    case TRAP_TRACE:
        break;
    default:
        std::cout << "Stopped on SIGTRAP\n";
        break;
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

bool Debugger::getConfirmation() {
    while(1) {
        std::string confirmation;
        std::cout << "Are you sure?(y/n)\n";
        std::cin >> confirmation;

        if (confirmation == "y") {
            return true;
        } else if (confirmation == "n") {
            return false;
        }
    }
}

void Debugger::prompt() {
    while(1) {
        const std::string input = getCommand();
        const auto tokens = getTokens(input);

        if (tokens.size() > 0) {
            if (tokens[0] == "c") {
                continueExecution();
                break;
            } else if (tokens[0] == "b") {
                addBreakpoint(tokens);
            } else if (tokens[0] == "d") {
                deleteBreakpoint(tokens);
            } else if (tokens[0] == "r") {
                displayRegisters();
            } else if (tokens[0] == "s") {
                singleStep();
                break;
            } else if (tokens[0] == "q") {
                if (getConfirmation()) {
                    std::cout << "Killing child process...\n";
                    ptrace(PTRACE_KILL, pid, nullptr, nullptr);
                    break;
                }
            } else {
                std::cout << "Invalid Command\n";
            }
        }
    }
}

void Debugger::continueExecution() {
    if (onBreakpoint) {
        auto instructionPtr = getRegisters().rip - 1;
        breakpointsMap[instructionPtr].disable();
        decrementInstructionPointer();
        // Single step so breakpoint can be re-enabled before continuing
        ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr);
        int status;
        waitpid(pid, &status, 0);
        breakpointsMap[instructionPtr].enable();
    }
    std::cout << "Continuing...\n";
    ptrace(PTRACE_CONT, pid, nullptr, 0);
}

void Debugger::singleStep() {
    if (onBreakpoint) {
        auto instructionPtr = getRegisters().rip - 1;
        breakpointsMap[instructionPtr].disable();
        decrementInstructionPointer();
        ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr);
        breakpointsMap[instructionPtr].enable();
    } else {
        ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr);
    }

    std::cout << "Single Stepping..." <<  std::endl;
}

void Debugger::addBreakpoint(const std::vector<std::string> tokens) {
    if (tokens.size() > 1) {
        if (std::regex_match(tokens[1], std::regex("0[xX][0-9a-fA-F]{1,16}"))) {
            auto addr = stol(tokens[1], 0, 16);
            setBreakpoint(addr);
        } else {
            std::cout << "Invalid address. Example: b 0x402a00\n";
        }
    } else {
        std::cout << "Specify address. Example: b 0x402a00\n";
    }
}

void Debugger::deleteBreakpoint(const std::vector<std::string> tokens) {
    if (tokens.size() > 1) {
        if (std::regex_match(tokens[1], std::regex("0[xX][0-9a-fA-F]{1,16}"))) {
            auto addr = stol(tokens[1], 0, 16);
            unsetBreakpoint(addr);
        } else {
            std::cout << "Invalid address. Example: d 0x402a00\n";
        }
    } else {
        std::cout << "Specify address. Example: d 0x402a00\n";
    }
}

// Split string into tokens using space as delimeter
std::vector<std::string> Debugger::getTokens(const std::string input) {
    std::istringstream inputStream{input};
    std::vector<std::string> tokens{std::istream_iterator<std::string>{inputStream}, std::istream_iterator<std::string>{}};
    return tokens;
}

void Debugger::setBreakpoint(const std::uintptr_t address) {
    auto search = breakpointsMap.find(address);

    if (search == breakpointsMap.end() || !breakpointsMap[address].isEnabled()) {
        Breakpoint breakpoint{pid, address};
        breakpoint.enable();
        breakpointsMap[address] = breakpoint;
        std::cout << "Breakpoint set at 0x" << std::hex << address << "\n";
    } else {
        std::cout << "Breakpoint already set at 0x" << std::hex << address << "\n";
    }
}

void Debugger::unsetBreakpoint(const std::uintptr_t address) {
    auto search = breakpointsMap.find(address);

    if (search != breakpointsMap.end() && breakpointsMap[address].isEnabled()) {
        breakpointsMap[address].disable();
        // Fix instruction pointer if currently stopped on this breakpoint
        auto prevIntructionAddr = getRegisters().rip - 1;

        if (address == prevIntructionAddr) {
            decrementInstructionPointer();
        }
        std::cout << "Removed breakpoint at 0x" << std::hex << address << "\n";
    } else {
        std::cout << "No Breakpoint set at 0x" << std::hex << address << "\n";
    }
}

void Debugger::decrementInstructionPointer() {
    user_regs_struct regs = getRegisters();
    regs.rip--;
    ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
}

user_regs_struct Debugger::getRegisters() {
    user_regs_struct registers;
    ptrace(PTRACE_GETREGS, pid, nullptr, &registers);
    return registers;
}

void Debugger::displayRegisters() {
    user_regs_struct registers = getRegisters();
    std::cout << "RAX: " << uintToHexStr(registers.rax, 16) << "    R8:  " << uintToHexStr(registers.r8, 16) << "\n"
              << "RBX: " << uintToHexStr(registers.rbx, 16) << "    R9:  " << uintToHexStr(registers.r9, 16) << "\n"
              << "RCX: " << uintToHexStr(registers.rcx, 16) << "    R10: " << uintToHexStr(registers.r10, 16) << "\n"
              << "RDX: " << uintToHexStr(registers.rdx, 16) << "    R11: " << uintToHexStr(registers.r11, 16) << "\n"
              << "RSP: " << uintToHexStr(registers.rsp, 16) << "    R12: " << uintToHexStr(registers.r12, 16) << "\n"
              << "RBP: " << uintToHexStr(registers.rbp, 16) << "    R13: " << uintToHexStr(registers.r13, 16) << "\n"
              << "RSI: " << uintToHexStr(registers.rsi, 16) << "    R14: " << uintToHexStr(registers.r14, 16) << "\n"
              << "RDI: " << uintToHexStr(registers.rdi, 16) << "    R15: " << uintToHexStr(registers.r15, 16) << "\n"
              << "RIP: " << uintToHexStr(registers.rip, 16) << "\n"
              ;
}

std::string Debugger::uintToHexStr(const uintptr_t num, const unsigned int len) {
    std::stringstream ss;
    ss << std::hex << num;
    auto hexString = ss.str();
    padString(hexString, '0', len);

    return hexString;
}

void Debugger::padString(std::string &str, const char c, const unsigned int len) {
    if (str.length() < len) {
        str.insert(0, len - str.length(), c);
    }
}
