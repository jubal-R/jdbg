#ifndef CORE_H
#define CORE_H

#include "debugger.h"

class Core
{
public:
    Core(pid_t pid);
    void run();

private:
    Debugger *debugger;

    std::string getCommand();
    std::vector<std::string> getTokens(const std::string input);
    void handleInput(std::vector<std::string> tokens);
    bool getConfirmation();
    bool tryParseAddress(std::string addrString, long *address);
    void addBreakpoint(const std::vector<std::string> tokens);
    void deleteBreakpoint(const std::vector<std::string> tokens);
    void displayRegisters();
    void padString(std::string &str, const char c, const unsigned int len);
    std::string uintToHexStr(const uintptr_t num, const unsigned int len);
    void handleSignal(int signal);
};

#endif // CORE_H
