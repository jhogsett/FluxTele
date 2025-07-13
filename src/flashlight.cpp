#include "saved_data.h"
#include "flashlight.h"
#include "utils.h"
#include "buffers.h"
#include "signal_meter.h"

// External reference to signal meter (defined in main.cpp)
extern SignalMeter signal_meter;

Flashlight::Flashlight(const char *title) : Option(title)
{
}

void Flashlight::next_option(){
    option_flashlight += 5;  // Increment by 5 for faster adjustment
    if(option_flashlight > MAX_FLASHLIGHT)
        option_flashlight = MAX_FLASHLIGHT;
    
    // Update signal meter LEDs to show white at current flashlight level
    if(option_flashlight > 0) {
        signal_meter.set_flashlight_mode(option_flashlight);
    } else {
        signal_meter.clear_flashlight_mode();
    }
}

void Flashlight::prev_option(){
    option_flashlight -= 5;  // Decrement by 5 for faster adjustment
    if(option_flashlight < MIN_FLASHLIGHT)
        option_flashlight = MIN_FLASHLIGHT;
    
    // Update signal meter LEDs to show white at current flashlight level
    if(option_flashlight > 0) {
        signal_meter.set_flashlight_mode(option_flashlight);
    } else {
        signal_meter.clear_flashlight_mode();
    }
}

void Flashlight::update_display(HT16K33Disp *display){
    if(option_flashlight == 0) {
        sprintf(display_text_buffer, "OFF");
    } else {
        sprintf(display_text_buffer, "FLuX %3d", option_flashlight);  // Right-align the number in 3 characters
    }
    display->scroll_string(display_text_buffer, 1, 1);

    // Update signal meter LEDs to reflect current flashlight setting
    // This ensures the LEDs show the correct state when the option is first displayed
    if(option_flashlight > 0) {
        signal_meter.set_flashlight_mode(option_flashlight);
    } else {
        signal_meter.clear_flashlight_mode();
    }
}
