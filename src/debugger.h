#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "breakpoint.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <sys/types.h>
#include <sys/user.h>

class Debugger
{
public:
    Debugger(pid_t p)
        : pid(p){}
    static void setupInferior(const char* path);
    void debug();

private:
    pid_t pid;
    std::unordered_map<std::uintptr_t, Breakpoint> breakpointsMap;

    void prompt();
    void setBreakpoint(std::uintptr_t address);
    void unsetBreakpoint(std::uintptr_t address);
    void checkForBreakpoint();
    void handleContinue();
    void displayRegisters();
    void handleBreakpoint(std::vector<std::string> tokens);
    void handleDeleteBreakpoint(std::vector<std::string> tokens);
    void padString(std::string &str, char c, unsigned int len);
    bool getConfirmation();
    std::string getCommand();
    std::string uintToHexStr(uintptr_t num, unsigned int len);
    std::vector<std::string> getTokens(std::string input);
    user_regs_struct getRegisters();
};

#endif // DEBUGGER_H
