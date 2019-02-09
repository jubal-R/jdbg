#include <iostream>
#include <unistd.h>

#include "core.h"

int main(int argc, char* argv[])
{
    /*
    if (argc < 2)
    {
        std::cout << "Must specify program to debug." << std::endl;
        return -1;
    }*/

    auto target = "ls";

    auto pid = fork();

    if (pid == 0)
    {
        Debugger::setupInferior(target);
    }
    else if (pid >= 1)
    {
        Core core {pid};
        core.run();
    }

    return 0;
}
