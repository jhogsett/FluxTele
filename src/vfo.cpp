#include <MD_AD9833.h>
#include "wavegen.h"
#include "vfo.h"
#include "buffers.h"
#include "signal_meter.h"
#include "station_config.h"
#include "saved_data.h"  // For option_bfo_offset

VFO::VFO(const char *title, float frequency, unsigned long step, RealizationPool *realization_pool) : Mode(title)
{
    _frequency = long(frequency);
    _sub_frequency = int((frequency - _frequency) * 10.0);
    _step = step;
    _realization_pool = realization_pool;
}

// step needs to be in 0.1Hz units
// when step is 0.1Hz, use xxxxxxx.y format
void VFO::update_display(HT16K33Disp *display){
    if(_frequency >= 100000000L){
        // Display as 2450.0000 in MHz
        int megpart = _frequency / 1000000L;
        long decpart = _frequency - (megpart * 1000000L);
        int decparti = decpart / 100L;
        
        sprintf(display_text_buffer, "%4d.%04d", megpart, decparti);
        
    } else if(_frequency >= 1000000L) {
        // Display 7,015,089 as 7015.089 in KHz
        int megpart = _frequency / 1000000L;
        long remainder = _frequency - (megpart * 1000000L);
        int kilpart = remainder / 1000L;
        remainder = remainder - (kilpart * 1000L);
        int unipart = remainder;
        
        sprintf(display_text_buffer, "%2d.%03d.%03d", megpart, kilpart, unipart);
        
    } else {
        // Display in Hz
        sprintf(display_text_buffer, "%8ld", _frequency);    }

    display->show_string(display_text_buffer);
}

void VFO::update_realization(){
    _realization_pool->update(this);
}

void VFO::force_transmitter_refresh(){
    // Force hardware refresh for all SimTransmitter objects
    // This ensures wave generators are properly updated when switching to SimRadio
    _realization_pool->force_sim_transmitter_refresh();
}

void VFO::mark_hardware_dirty(){
    // Mark hardware state as unknown - will trigger refresh on next update
    _realization_pool->mark_dirty();
}

void VFO::update_signal_meter(SignalMeter *signal_meter) {
    // Calculate signal strength based on proximity to active stations    // Send charge pulses instead of setting signal strength directly
    
    // Note: Charge pulses are now sent directly by stations when their carrier turns on
    // This provides more realistic signal meter behavior tied to actual transmission events
}

// Static utility for stations to calculate signal strength charge based on VFO proximity
int VFO::calculate_signal_charge(float station_freq, float vfo_freq) {
    // Calculate frequency difference using same method as audio system
    // Apply BFO offset so meter responds to full receiver passband, not just positive audio
    float freq_diff = vfo_freq - (station_freq - option_bfo_offset);
    
    // Signal strength calculation:
    // - Charge starts when station enters receiver passband (at BFO offset below station)
    // - Range 0 to +5000 Hz matching audio system behavior
    // - Perfect consistency: signal meter matches receiver passband
    
    const float MAX_RANGE = 5000.0;  // Match MAX_AUDIBLE_FREQ for perfect consistency
    
    // Respond to receiver passband: BFO offset below station frequency to +5000 Hz above
    if (freq_diff >= 0 && freq_diff <= MAX_RANGE) {
        // Calculate proximity factor (0.0 to 1.0)
        float proximity = 1.0 - (freq_diff / MAX_RANGE);
        
        // Apply squared curve for realistic but not too steep falloff
        proximity = proximity * proximity;
        
        // Convert proximity to charge amount (much reduced for proportionality)
        // Lower charge amounts prevent meter from jumping too high
        int charge = (int)(proximity * 2.0);  // 0-2 charge amount (much reduced)
        
        return charge;
    }
    
    return 0;  // No charge if out of receiver passband
}
