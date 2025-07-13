#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include "mode.h"

class Option : public Mode
{
public:
    // constructor
    Option(const char *title);

    virtual void next_option();
    virtual void prev_option();

private:
};

#endif // __OPTIONS_H__
