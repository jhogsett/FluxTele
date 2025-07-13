#ifndef __BFO_HANDLER_H__
#define __BFO_HANDLER_H__

#include <HT16K33Disp.h>
#include "mode.h"
#include "mode_handler.h"

class BFOHandler : public ModeHandler
{
public:
    // constructor
    BFOHandler(Mode * mode);

    virtual bool event_sink(int event, int count);
    virtual bool event_sink(bool pressed, bool long_pressed);

private:
};

#endif // __BFO_HANDLER_H__
