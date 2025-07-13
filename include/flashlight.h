#ifndef __FLASHLIGHT_H__
#define __FLASHLIGHT_H__

#include "option.h"

#define MIN_FLASHLIGHT 0
#define MAX_FLASHLIGHT 255

class Flashlight : public Option
{
public:
    // constructor
    Flashlight(const char *title);

    virtual void next_option();
    virtual void prev_option();
    
    virtual void update_display(HT16K33Disp *display);

private:
};

#endif
