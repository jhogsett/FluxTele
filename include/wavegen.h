#ifndef __WAVEGEN_H__
#define __WAVEGEN_H__

#include <MD_AD9833.h>

class WaveGen
{
public:
    WaveGen(MD_AD9833 * sig_gen);

    void set_frequency(float frequency, bool main=true);
    void set_active_frequency(bool main);
    void force_refresh();  // Force hardware update regardless of cached state

    MD_AD9833 * _sig_gen;
    float _frequency_main;
    float _frequency_alt;
    bool _main;
};

#endif
