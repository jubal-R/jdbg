#include "debugger.h"
#include "breakpoint.h"

#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>

void Debugger::setupInferior(const char *path)
{
    ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
//    execl(path, path, nullptr);
    execl("testbin", "testbin", nullptr);
}

int Debugger::waitForSignal()
{
    onBreakpoint = false;
    int status;
    waitpid(pid, &status, 0);

    if (WIFSTOPPED(status))
    {
        switch (WSTOPSIG(status))
        {
            case SIGTRAP:
                return handleSIGTRAP();
            case SIGILL:
                return sig_ill;
            case SIGSEGV:
                return sig_segv;
            default:
                return sig_stopped;
        }
    }
    else if (WIFEXITED(status))
    {
        return sig_exited;
    }
    else if (WIFSIGNALED(status))
    {
        return sig_terminated;
    }

    return sig_unknown;
}

int Debugger::handleSIGTRAP()
{
    siginfo_t sigInfo = getSigInfo();
    switch (sigInfo.si_code)
    {
        // Breakpoint
        case TRAP_BRKPT:
        case SI_KERNEL:
            onBreakpoint = true;
            return sig_breakpoint;
        // Single Step
        case TRAP_TRACE:
            return sig_singlestep;
        default:
            return sig_trap;
   }
}

void Debugger::killInferior()
{
    ptrace(PTRACE_KILL, pid, nullptr, nullptr);
}

siginfo_t Debugger::getSigInfo()
{
    siginfo_t sigInfo;
    ptrace(PTRACE_GETSIGINFO, pid, nullptr, &sigInfo);
    return sigInfo;
}

void Debugger::continueExecution()
{
    if (onBreakpoint)
    {
        auto instructionPtr = getRegisters().rip - 1;
        breakpointsMap[instructionPtr].disable();
        decrementInstructionPointer();
        // Single step so breakpoint can be re-enabled before continuing
        ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr);
        int status;
        waitpid(pid, &status, 0);
        breakpointsMap[instructionPtr].enable();
    }

    ptrace(PTRACE_CONT, pid, nullptr, 0);
}

void Debugger::singleStep()
{
    if (onBreakpoint)
    {
        auto instructionPtr = getRegisters().rip - 1;
        breakpointsMap[instructionPtr].disable();
        decrementInstructionPointer();
        ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr);
        breakpointsMap[instructionPtr].enable();
    }
    else
    {
        ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr);
    }
}

void Debugger::setBreakpoint(const std::uintptr_t address)
{
    auto search = breakpointsMap.find(address);

    if (search == breakpointsMap.end() || !breakpointsMap[address].isEnabled())
    {
        Breakpoint breakpoint{pid, address};
        breakpoint.enable();
        breakpointsMap[address] = breakpoint;

    }
}

void Debugger::unsetBreakpoint(const std::uintptr_t address)
{
    auto search = breakpointsMap.find(address);

    if (search != breakpointsMap.end() && breakpointsMap[address].isEnabled())
    {
        breakpointsMap[address].disable();
        // Fix instruction pointer if currently stopped on this breakpoint
        auto prevIntructionAddr = getRegisters().rip - 1;

        if (address == prevIntructionAddr)
            decrementInstructionPointer();
    }
}

void Debugger::decrementInstructionPointer()
{
    user_regs_struct regs = getRegisters();
    regs.rip--;
    ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
}

user_regs_struct Debugger::getRegisters()
{
    user_regs_struct registers;
    ptrace(PTRACE_GETREGS, pid, nullptr, &registers);
    return registers;
}
