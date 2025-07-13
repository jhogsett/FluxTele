#ifndef __CONTRAST_HANDLER_H__
#define __CONTRAST_HANDLER_H__

#include <HT16K33Disp.h>
#include "mode.h"
#include "mode_handler.h"

class ContrastHandler : public ModeHandler
{
public:
    // constructor
    ContrastHandler(Mode * mode);

    virtual bool event_sink(int event, int count);
    virtual bool event_sink(bool pressed, bool long_pressed);

private:
};

#endif // __CONTRAST_HANDLER_H__
