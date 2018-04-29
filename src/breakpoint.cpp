#include "breakpoint.h"

#include <sys/ptrace.h>
#include <iostream>

bool Breakpoint::isEnabled() {
    return enabled;
}

uintptr_t Breakpoint::getAdress() {
    return address;
}

void Breakpoint::enable() {
    originalData = ptrace(PTRACE_PEEKDATA, pid, address, nullptr);
    // Binary AND with ~0xff clears first byte of word; Binary OR sets first byte to 0xcc after it is zeroed out
    uint64_t wordWithInt3 = (originalData & ~0xff) | 0xcc;
    ptrace(PTRACE_POKEDATA, pid, address, wordWithInt3);
    enabled = true;
}

void Breakpoint::disable() {
    ptrace(PTRACE_POKEDATA, pid, address, originalData);
    enabled = false;
}
