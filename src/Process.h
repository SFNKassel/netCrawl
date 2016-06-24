#ifndef _PROCESS_H
#define _PROCESS_H

#include <string>
class Process {
public:
    Process(std::string programm) {
        system(programm.c_str());
    }
};

#endif
