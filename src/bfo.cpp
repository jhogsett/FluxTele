#include "saved_data.h"
#include "bfo.h"
#include "utils.h"
#include "buffers.h"
#include <stdlib.h>  // For itoa()
#include <string.h>  // For strcpy(), strcat()

BFO::BFO(const char *title) : Option(title)
{
}

void BFO::next_option(){
    option_bfo_offset += BFO_INCREMENT;
    if(option_bfo_offset > MAX_BFO_OFFSET)
        option_bfo_offset = MAX_BFO_OFFSET;
}

void BFO::prev_option(){
    option_bfo_offset -= BFO_INCREMENT;
    if(option_bfo_offset < MIN_BFO_OFFSET)
        option_bfo_offset = MIN_BFO_OFFSET;
}

void BFO::update_display(HT16K33Disp *display){
    // No hardware adjustment needed for BFO (unlike contrast which affects display hardware)
    // Manual string building instead of sprintf to save Flash
    char freq_str[6];
    itoa(option_bfo_offset, freq_str, 10);
    strcpy(display_text_buffer, freq_str);
    strcat(display_text_buffer, " Hz");
    display->scroll_string(display_text_buffer, 1, 1);
}
