#include "../include/basic_types.h"
#include <EEPROM.h>
#include "../include/saved_data.h"

int option_contrast = DEFAULT_CONTRAST;
int option_bfo_offset = DEFAULT_BFO_OFFSET;
int option_flashlight = DEFAULT_FLASHLIGHT;

void load_save_data(){
	SavedData saved_data;
	EEPROM.get(0, saved_data);

	if(saved_data.version != SAVE_DATA_VERSION){
		reset_options();
		return;
	}
	option_contrast = saved_data.option_contrast;
	option_bfo_offset = saved_data.option_bfo_offset;
	option_flashlight = saved_data.option_flashlight;

	// ##DATA Load new persisted play data variables into memory here
}

void save_data(){	SavedData saved_data;	saved_data.version = SAVE_DATA_VERSION;
	saved_data.option_contrast = option_contrast;
	saved_data.option_bfo_offset = option_bfo_offset;
	saved_data.option_flashlight = option_flashlight;

	EEPROM.put(0, saved_data);
}

typedef void (*VoidFunc)(void);

void reset_device(){
	VoidFunc p = NULL;
	p();
}

bool reset_options(){
	option_contrast = DEFAULT_CONTRAST;
	option_bfo_offset = DEFAULT_BFO_OFFSET;

	save_data();

	reset_device();
	return false;
}
