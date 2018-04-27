#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <iostream>
#include <sys/types.h>

class Debugger
{
public:
    Debugger(pid_t p);
    static void setupInferior(const char* path);
    void debug();
    void prompt();
    std::string getCommand();

private:
    pid_t pid;
};

#endif // DEBUGGER_H
