#include <Arduino.h>
#include "../include/buffers.h"
#include "../include/saved_data.h"
#include "../include/utils.h"

char * load_f_string(const __FlashStringHelper* f_string, char *override_buffer){
	const char *p = (const char PROGMEM *)f_string;
	return strcpy_P(override_buffer ? override_buffer : fstring_buffer, p);
}

void random_unique(int count, int max_value, int *result){
    for(int i = 0; i < count; i++){
        bool found = false;
        while(!found){
            result[i] = random(max_value);
            found = true;
            for(int j = 0; j < i; j++){
                if(result[i] == result[j]){
                    found = false;
                    break;
                }
            }
        }
    }
}
