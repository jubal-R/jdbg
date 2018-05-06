#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "breakpoint.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <sys/types.h>
#include <sys/user.h>
#include <signal.h>

class Debugger
{
public:
    Debugger(pid_t p)
        : pid(p), onBreakpoint(false){}
    static void setupInferior(const char* path);
    void debug();

private:
    pid_t pid;
    std::unordered_map<std::uintptr_t, Breakpoint> breakpointsMap;
    bool onBreakpoint;

    void prompt();
    void handleSIGTRAP();
    void setBreakpoint(const std::uintptr_t address);
    void unsetBreakpoint(const std::uintptr_t address);
    void continueExecution();
    void singleStep();
    void decrementInstructionPointer();
    void displayRegisters();
    void addBreakpoint(const std::vector<std::string> tokens);
    void deleteBreakpoint(const std::vector<std::string> tokens);
    void padString(std::string &str, const char c, const unsigned int len);
    bool getConfirmation();
    std::string getCommand();
    std::string uintToHexStr(const uintptr_t num, const unsigned int len);
    std::vector<std::string> getTokens(const std::string input);
    user_regs_struct getRegisters();
    siginfo_t getSigInfo();
};

#endif // DEBUGGER_H
