#include <MD_AD9833.h>
#include "wavegen.h"

#define SILENT_FREQ 0.1

WaveGen::WaveGen(MD_AD9833 * sig_gen)
{
    _sig_gen = sig_gen;
	_frequency_main = SILENT_FREQ;
	_frequency_alt = SILENT_FREQ;
	_main = true;
}

void WaveGen::set_frequency(float frequency, bool main){
	bool update = false;
	if(main){
		if(_frequency_main != frequency){
			_frequency_main = frequency;
			update = true;
		}
	} else {
		if(_frequency_alt != frequency){
			_frequency_alt = frequency;
			update = true;
		}
	}

	if(update)
		_sig_gen->setFrequency((MD_AD9833::channel_t)(main ? 0 : 1), frequency);
}

void WaveGen::set_active_frequency(bool main){
	_sig_gen->setActiveFrequency((MD_AD9833::channel_t)(main ? 0 : 1));
	_main = main;
}

void WaveGen::force_refresh(){
	// Force hardware update regardless of cached state
	// This is needed when returning to SimRadio after application switches
	// that may have affected the AD9833 hardware state
	_sig_gen->setFrequency((MD_AD9833::channel_t)(0), _frequency_main);
	_sig_gen->setFrequency((MD_AD9833::channel_t)(1), _frequency_alt);
	_sig_gen->setActiveFrequency((MD_AD9833::channel_t)(_main ? 0 : 1));
}
