#ifndef __UTILS_H__
#define __UTILS_H__

#include <Arduino.h>

// shortcut for using strings stored in program memory
#define FSTR(s) (load_f_string(F(s)))

extern char * load_f_string(const __FlashStringHelper* f_string, char *override_buffer=NULL);
extern void random_unique(int count, int max_value, int *result);

#endif
