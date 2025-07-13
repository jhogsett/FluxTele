#ifndef __FLASHLIGHT_HANDLER_H__
#define __FLASHLIGHT_HANDLER_H__

#include "option_handler.h"

class FlashlightHandler : public Option_Handler
{
public:
    FlashlightHandler(Mode * mode);
};

#endif
