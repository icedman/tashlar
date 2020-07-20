#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <string>

struct scripting_t {

    scripting_t();
    ~scripting_t();

    static scripting_t* instance();

    void initialize();
    int runScript(std::string script);
};

#endif // SCRIPTING_H
