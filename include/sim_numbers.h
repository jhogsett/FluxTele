#ifndef __SIM_NUMBERS_H__
#define __SIM_NUMBERS_H__

#include "async_morse.h"
#include "sim_transmitter.h"

class SignalMeter; // Forward declaration

#define NUMBERS_SPACE_FREQUENCY 0.1
#define DEFAULT_INTERVAL_REPEATS 6  // Number of "FT" interval signals for optimal anticipation

/**
 * Simulated Numbers Station - transmits creepy 5-digit number groups in Morse code
 * Generates mysterious transmissions like: "74921   88315   " (pure digits only, no punctuation)
 */
class SimNumbers : public SimTransmitter
{
public:
    SimNumbers(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq, int wpm = 18);
    virtual bool begin(unsigned long time) override;
    
    virtual bool update(Mode *mode) override;
    virtual bool step(unsigned long time) override;

    void realize();

private:    void generate_next_number_group();
    void generate_interval_signal();
    void generate_ending_sequence();
    void apply_frequency_drift();   // Add slight frequency drift for creepiness
      AsyncMorse _morse;
    char _group_buffer[6];          // Buffer for single 5-digit group + null ("12345")
    int _groups_sent;               // Count of groups sent in current cycle
    int _total_groups_per_cycle;    // Total groups to send per cycle (13 for creepiness)
    bool _in_inter_group_delay;     // True when waiting between groups
    unsigned long _next_group_time; // When to send next group
    bool _transmission_active;      // Track if morse is currently transmitting
    int _wpm;                       // Store WPM setting for consistent use
    SignalMeter *_signal_meter;     // Pointer to signal meter for charge pulses
    // Note: Using base class _fixed_freq instead of redundant _stored_fixed_freq
    
    // Enhanced numbers station state
    enum NumbersPhase {
        PHASE_INTERVAL_SIGNAL,      // Sending "FT FT FT..." before numbers
        PHASE_NUMBERS,              // Sending the 13 groups of numbers
        PHASE_ENDING,               // Sending "00000" ending sequence
        PHASE_CYCLE_DELAY           // Waiting between complete cycles
    };
    
    NumbersPhase _current_phase;
    int _interval_repeats_sent;     // Count of "FT" repeats sent
    int _total_interval_repeats;    // Total "FT" repeats to send (e.g., 6 for authentic feel)
};

#endif
