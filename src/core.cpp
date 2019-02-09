#include "core.h"

#include <readline/readline.h>
#include <readline/history.h>
#include <iostream>
#include <sstream>
#include <iterator>
#include <regex>

Core::Core(pid_t pid)
{
    debugger = new Debugger {pid};
}

void Core::run()
{
    while (1)
    {
        const std::string input = getCommand();
        const auto tokens = getTokens(input);
        if (tokens.size() > 0)
            handleInput(tokens);
    }
}

void Core::handleInput(std::vector<std::string> tokens)
{
    if (tokens[0] == "c")
    {
        std::cout << "Continuing...\n";
        debugger->continueExecution();
        auto signal = debugger->waitForSignal();
        handleSignal(signal);
    }
    else if (tokens[0] == "b")
    {
        addBreakpoint(tokens);
    }
    else if (tokens[0] == "d")
    {
        deleteBreakpoint(tokens);
    }
    else if (tokens[0] == "r")
    {
        displayRegisters();
    }
    else if (tokens[0] == "s")
    {
        std::cout << "Single Stepping..." <<  std::endl;
        debugger->singleStep();
    }
    else if (tokens[0] == "q")
    {
        if (getConfirmation())
        {
            std::cout << "Killing child process...\n";
            debugger->killInferior();
            exit(0);
        }
    }
    else
        std::cout << "Invalid Command\n";
}

std::string Core::getCommand()
{
    char* line = nullptr;
    line = readline("jdbg> ");

    if (strlen(line) > 0)
        add_history(line);

    std::string command(line);
    free(line);
    return command;
}

// Split string into tokens using space as delimeter
std::vector<std::string> Core::getTokens(const std::string input)
{
    std::istringstream inputStream{input};
    std::vector<std::string> tokens{std::istream_iterator<std::string>{inputStream}, std::istream_iterator<std::string>{}};
    return tokens;
}

bool Core::getConfirmation()
{
    while (1)
    {
        std::string confirmation;
        std::cout << "Are you sure?(y/n)\n";
        std::cin >> confirmation;

        if (confirmation == "y")
            return true;
        else if (confirmation == "n")
            return false;
    }
}

bool Core::tryParseAddress(std::string addrString, long *address)
{
    if (std::regex_match(addrString, std::regex("0[xX][0-9a-fA-F]{1,16}")))
    {
        *address = stol(addrString, 0, 16);
        return true;
    }
    else
        return false;
}

void Core::addBreakpoint(const std::vector<std::string> tokens)
{
    if (tokens.size() > 1)
    {
        long addr;
        if (tryParseAddress(tokens[1], &addr))
        {
            debugger->setBreakpoint(addr);
            std::cout << "Breakpoint set at 0x" << std::hex << addr << "\n";
        }
        else
            std::cout << "Invalid address. Example: b 0x402a00\n";
    }
    else
        std::cout << "Specify address. Example: b 0x402a00\n";
}

void Core::deleteBreakpoint(const std::vector<std::string> tokens)
{
    if (tokens.size() > 1)
    {
        long addr;
        if (tryParseAddress(tokens[1], &addr))
        {
            std::cout << "Removing breakpoint at 0x" << std::hex << addr << "\n";
            debugger->unsetBreakpoint(addr);
        }
        else
            std::cout << "Invalid address. Example: d 0x402a00\n";
    }
    else
        std::cout << "Specify address. Example: d 0x402a00\n";
}

void Core::displayRegisters()
{
    user_regs_struct registers = debugger->getRegisters();
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

std::string Core::uintToHexStr(const uintptr_t num, const unsigned int len)
{
    std::stringstream ss;
    ss << std::hex << num;
    auto hexString = ss.str();
    padString(hexString, '0', len);
    return hexString;
}

void Core::padString(std::string &str, const char c, const unsigned int len)
{
    if (str.length() < len) {
        str.insert(0, len - str.length(), c);
    }
}

void Core::handleSignal(int signal)
{
    switch (signal)
    {
        case Debugger::sig_trap:
            std::cout << "Stopped on SIGTRAP\n";
            break;
        case Debugger::sig_ill:
            std::cout << "Stopped on SIGILL\n";
            break;
        case Debugger::sig_segv:
            std::cout << "Stopped on SIGSEGV\n";
            break;
        case Debugger::sig_exited:
            std::cout << "Target terminated - exiting\n";
            exit(0);
            break;
        case Debugger::sig_terminated:
            std::cout << "Process terminated by signal\n";
            exit(0);
            break;
        case Debugger::sig_breakpoint:
            std::cout << "Stopped on breakpoint\n";
            break;
        default:
            break;
    }
}
