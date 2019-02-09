#include "breakpoint.h"

#include <sys/ptrace.h>

bool Breakpoint::isEnabled()
{
    return enabled;
}

uintptr_t Breakpoint::getAddress()
{
    return address;
}

void Breakpoint::enable()
{
    auto word = ptrace(PTRACE_PEEKDATA, pid, address, nullptr);
    originalByte = static_cast<uint8_t>(word & 0xff);
    // Binary AND with ~0xff clears first byte of word; Binary OR sets first byte to 0xcc after it is zeroed out
    uint64_t wordWithInt3 = (word & ~0xff) | 0xcc;
    ptrace(PTRACE_POKEDATA, pid, address, wordWithInt3);
    enabled = true;
}

void Breakpoint::disable()
{
    // Only restore single byte as word may contian additional breakpoints
    auto word = ptrace(PTRACE_PEEKDATA, pid, address, nullptr);
    uint64_t restoredWord = (word & ~0xff) | originalByte;
    ptrace(PTRACE_POKEDATA, pid, address, restoredWord);
    enabled = false;
}
