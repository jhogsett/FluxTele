#ifndef __BFO_H__
#define __BFO_H__

#include "option.h"

// BFO offset range: 0 Hz to 2000 Hz in increments
#define MIN_BFO_OFFSET 0
#define MAX_BFO_OFFSET 2000
#define BFO_INCREMENT 50     // 50 Hz increments for fine control

class BFO : public Option
{
public:
    // constructor
    BFO(const char *title);

    virtual void next_option();
    virtual void prev_option();
    
    virtual void update_display(HT16K33Disp *display);

private:
};

#endif
