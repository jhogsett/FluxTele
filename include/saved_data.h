#ifndef __SAVED_DATA_H
#define __SAVED_DATA_H

#include "basic_types.h"

// when adding new persisted play data, search for ##DATA

// ##DATA Increment the save data version to force upgraded devices to auto-reset after programming
// Current save data version
// On start-up if this differs from the EEPROM value, the data is reset to defaults
#define SAVE_DATA_VERSION 3   // Incremented for flashlight option addition

#define DEFAULT_CONTRAST 2
#define DEFAULT_BFO_OFFSET 700   // 700 Hz default BFO offset for comfortable audio tuning
#define DEFAULT_FLASHLIGHT 0     // Flashlight starts at 0 (off)

// the longest possible count of milliseconds
#define DEFAULT_TIME ((unsigned long)-1)

// Display time for interstitial displays during games
#define ROUND_DELAY 750

extern byte save_data_version;

extern int option_contrast;
extern int option_bfo_offset;  // BFO offset in Hz (0-2000)
extern int option_flashlight;  // Flashlight brightness (0-255)

// Saved data structure version 1
struct SavedData{
	byte version;
	int option_contrast;
	int option_bfo_offset;
	int option_flashlight;
};

extern void load_save_data();
extern void save_data();
extern bool reset_options();
extern void reset_device();

#endif
