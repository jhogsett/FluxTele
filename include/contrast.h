#ifndef __CONTRAST_H__
#define __CONTRAST_H__

#include "option.h"

#define MIN_CONTRAST 0
#define MAX_CONTRAST 15

class Contrast : public Option
{
public:
    // constructor
    Contrast(const char *title);

    virtual void next_option();
    virtual void prev_option();
    
    virtual void update_display(HT16K33Disp *display);

private:
};

#endif
