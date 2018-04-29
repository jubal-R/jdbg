#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include <string>

class Breakpoint
{
public:
    Breakpoint() = default;
    Breakpoint(pid_t p, std::uintptr_t addr)
        : pid(p), address(addr), originalData(), enabled(false){}
    bool isEnabled();
    uintptr_t getAdress();
    void enable();
    void disable();
private:
    pid_t pid;
    std::uintptr_t address;
    std::uintptr_t originalData;
    bool enabled;
};

#endif // BREAKPOINT_H
