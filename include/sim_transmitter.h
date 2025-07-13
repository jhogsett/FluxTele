#ifndef __SIM_TRANSMITTER_H__
#define __SIM_TRANSMITTER_H__

#include "signal_meter.h"
#include "vfo.h"
#include "realization.h"
#include "wave_gen_pool.h"

// Station states for dynamic station management
enum StationState {
    DORMANT,     // No frequency assigned, minimal memory usage
    ACTIVE,      // Frequency assigned, tracking VFO proximity  
    AUDIBLE,     // Active + has AD9833 generator assigned
    SILENT       // Active but no AD9833 (>4 stations in range)
};

// Common constants for simulated transmitters
#define MAX_AUDIBLE_FREQ 5000.0
#define MIN_AUDIBLE_FREQ 150.0
#define SILENT_FREQ 0.1

// BFO (Beat Frequency Oscillator) offset for comfortable audio tuning
// This shifts the audio frequency without affecting signal meter calculations
// Now dynamically adjustable via option_bfo_offset (0-2000 Hz)
// #define BFO_OFFSET 700.0   // Replaced by dynamic option_bfo_offset

/**
 * Base class for simulated transmitters (CW/RTTY).
 * Provides common functionality and interface for station simulation.
 * 
 * IMPORTANT: begin() and end() are designed to be idempotent and safe for repeated calls.
 * This supports dynamic station management where stations may be restarted with new
 * frequencies or reallocated to different wave generators. The pattern end() followed
 * by begin() properly reinitializes the station with any frequency changes.
 */
class SimTransmitter : public Realization
{
public:
    SimTransmitter(WaveGenPool *wave_gen_pool, float fixed_freq = 0.0);
    
    virtual bool step(unsigned long time) = 0;  // Pure virtual - must be implemented by derived classes
    virtual void end();  // Common cleanup logic
    virtual void force_wave_generator_refresh() override;  // Override base class method

    // Dynamic station management methods
    virtual bool reinitialize(unsigned long time, float fixed_freq);  // Reinitialize with new frequency
    virtual void randomize();  // Re-randomize station properties (callsign, WPM, etc.) - default implementation does nothing
    void set_station_state(StationState new_state);  // Change station state
    StationState get_station_state() const;  // Get current station state
    bool is_audible() const;  // True if station has AD9833 generator assigned
    float get_fixed_frequency() const;  // Get station's target frequency
    void setActive(bool active);
    bool isActive() const;

protected:    // Common utility methods
    bool check_frequency_bounds();  // Returns true if frequency is in audible range
    bool common_begin(unsigned long time, float fixed_freq);  // Common initialization logic
    void common_frequency_update(Mode *mode);  // Common frequency calculation (mode must be VFO)
    void force_frequency_update();  // Immediately update wave generator after _fixed_freq changes// Common member variables
    float _fixed_freq;  // Target frequency for this station
    bool _enabled;      // True when frequency is in audible range
    float _frequency;   // Current frequency difference from VFO
    float _vfo_freq;    // Current VFO frequency (for signal meter charge calculation)
    bool _active;       // True when transmitter should be active
    
    // Dynamic station management state
    StationState _station_state;  // Current state in dynamic management system

    // Centralized charge pulse logic for all simulated stations
    virtual void send_carrier_charge_pulse(SignalMeter* signal_meter) {
        if (!signal_meter) return;
        int charge = VFO::calculate_signal_charge(_fixed_freq, _vfo_freq);
        if (charge > 0) {
            const float LOCK_WINDOW_HZ = 50.0; // Lock window threshold (adjust as needed)
            float freq_diff = abs(_fixed_freq - _vfo_freq);
            if (freq_diff <= LOCK_WINDOW_HZ) {
                signal_meter->add_charge(-charge);
            } else {
                signal_meter->add_charge(charge);
            }
        }
    }
};

#endif