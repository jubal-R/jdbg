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
    enum signal { sig_trap, sig_ill, sig_segv, sig_exited, sig_terminated, sig_breakpoint, sig_singlestep, sig_stopped, sig_unknown };

    Debugger(pid_t p)
        : pid(p), onBreakpoint(false){}

    static void setupInferior(const char* path);
    void debug();
    void killInferior();
    void continueExecution();
    int waitForSignal();
    void singleStep();
    void setBreakpoint(const std::uintptr_t address);
    void unsetBreakpoint(const std::uintptr_t address);
    user_regs_struct getRegisters();

private:
    pid_t pid;
    std::unordered_map<std::uintptr_t, Breakpoint> breakpointsMap;
    bool onBreakpoint;
    int handleSIGTRAP();
    void decrementInstructionPointer();
    siginfo_t getSigInfo();
};

#endif // DEBUGGER_H
