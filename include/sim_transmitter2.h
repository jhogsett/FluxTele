#ifndef __SIM_TRANSMITTER2_H__
#define __SIM_TRANSMITTER2_H__

// Wave generator selection for dual generator development
#define ENABLE_GENERATOR_A  // Enable by default
#define ENABLE_GENERATOR_C  // Enable for duplication testing

// Test configuration: Offset Generator C by a small amount for verification
#define GENERATOR_C_TEST_OFFSET 100.0  // Hz offset for testing dual generator operation

#include "signal_meter.h"
#include "vfo.h"
#include "realization.h"
#include "wave_gen_pool.h"

// Station states for dynamic station management
enum StationState2 {
    DORMANT2,     // No frequency assigned, minimal memory usage
    ACTIVE2,      // Frequency assigned, tracking VFO proximity  
    AUDIBLE2,     // Active + has AD9833 generator assigned
    SILENT2       // Active but no AD9833 (>4 stations in range)
};

// Common constants for simulated transmitters
#define MAX_AUDIBLE_FREQ2 5000.0
#define MIN_AUDIBLE_FREQ2 -700.0   // FluxTele: No BFO required for telephony, allows full radio tuning range
#define SILENT_FREQ2 0.1

/*
 * FLUXTELE FREQUENCY ARCHITECTURE EXPLANATION:
 * 
 * Traditional radio receivers use heterodyning: RF signal + Local Oscillator = Audio IF
 * This requires BFO (Beat Frequency Oscillator) to shift audio into audible range.
 * MIN_AUDIBLE_FREQ was originally 150Hz assuming BFO would always add frequency.
 * 
 * FluxTele telephony operates differently:
 * - Telephone signals are already at baseband audio frequencies (350-1633 Hz)
 * - No heterodyning required - direct audio synthesis via AD9833
 * - BFO is optional for user tuning comfort, not architecturally required
 * 
 * Setting MIN_AUDIBLE_FREQ = -700Hz allows:
 * - Full radio dial behavior (tune signals completely out of audible range)
 * - BFO = 0Hz operation (pure telephony frequencies with no offset)
 * - Natural sub-audible tuning (like a real radio receiver)
 * 
 * This is the correct implementation, not a workaround.
 */

// BFO (Beat Frequency Oscillator) offset for comfortable audio tuning
// FluxTele telephony operates directly at baseband frequencies (no heterodyning needed)
// BFO offset is optional for user comfort but not required for proper operation
// Now dynamically adjustable via option_bfo_offset (0-2000 Hz)
// #define BFO_OFFSET 700.0   // Replaced by dynamic option_bfo_offset

/**
 * Base class for simulated transmitters (CW/RTTY) - Duplicate class for testing.
 * Provides common functionality and interface for station simulation.
 *
 * INHERITANCE CONTRACT: All concrete station classes MUST inherit from both:
 * - SimTransmitter2 (for StationManager compatibility)  
 * - Realization (for RealizationPool compatibility)
 * This dual inheritance enables zero-copy array sharing between managers.
 *
 * IMPORTANT: begin() and end() are designed to be idempotent and safe for repeated calls.
 * This supports dynamic station management where stations may be restarted with new
 * frequencies or reallocated to different wave generators. The pattern end() followed
 * by begin() properly reinitializes the station with any frequency changes.
 */
class SimTransmitter2 : public Realization
{
public:
    SimTransmitter2(WaveGenPool *wave_gen_pool, float fixed_freq = 0.0);
    
    virtual bool step(unsigned long time) = 0;  // Pure virtual - must be implemented by derived classes
    virtual void end();  // Common cleanup logic
    virtual void force_wave_generator_refresh() override;  // Override base class method

    // Dynamic station management methods
    virtual bool reinitialize(unsigned long time, float fixed_freq);  // Reinitialize with new frequency
    virtual void randomize();  // Re-randomize station properties (callsign, WPM, etc.) - default implementation does nothing
    void set_station_state(StationState2 new_state);  // Change station state
    StationState2 get_station_state() const;  // Get current station state
    bool is_audible() const;  // True if station has AD9833 generator assigned
    float get_fixed_frequency() const;  // Get station's target frequency
    void setActive(bool active);
    bool isActive() const;

protected:    // Common utility methods
    bool check_frequency_bounds();  // Returns true if frequency is in audible range
    bool common_begin(unsigned long time, float fixed_freq);  // Common initialization logic
    void common_frequency_update(Mode *mode);  // Common frequency calculation (mode must be VFO)
    void force_frequency_update();  // Immediately update wave generator after _fixed_freq changes

    // Shared station properties (independent of wave generator)
    float _fixed_freq;  // Target frequency for this station (shared between A and B)
    bool _enabled;      // True when frequency is in audible range (shared)
    bool _active;       // True when transmitter should be active (shared)
    float _vfo_freq;    // Current VFO frequency (shared - there's only one VFO)

#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    // Wave Generator A variables
    float _frequency;   // Current frequency difference from VFO
    
    // Dynamic station management state
    StationState2 _station_state;  // Current state in dynamic management system

    // Wave Generator C variables
    float _frequency_c;   // Current frequency difference from VFO
    
    // // Dynamic station management state
    // StationState2 _station_state_c;  // Current state in dynamic management system
#endif

    // Centralized charge pulse logic for all simulated stations
    virtual void send_carrier_charge_pulse(SignalMeter* signal_meter);
};

#endif
