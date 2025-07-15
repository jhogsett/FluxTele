#ifndef __SIM_EXCHANGE_H__
#define __SIM_EXCHANGE_H__

#include "sim_transmitter.h"
#include "realization.h"

class SignalMeter; // Forward declaration

// Telephone exchange signal types
enum ExchangeSignalType {
    EXCHANGE_RINGING,     // 440 Hz + 480 Hz, 2s on + 4s off
    EXCHANGE_BUSY,        // 480 Hz + 620 Hz, 0.5s on + 0.5s off  
    EXCHANGE_REORDER,     // 480 Hz + 620 Hz, 0.25s on + 0.25s off
    EXCHANGE_DIAL_TONE,   // 350 Hz + 440 Hz, 30s on + 2s off
    EXCHANGE_ERROR,       // Complex 3-tone sequence
    EXCHANGE_SILENT       // No signal
};

// Standard North American telephony frequencies
#define EXCHANGE_DIAL_TONE_LOW     350.0    // Hz
#define EXCHANGE_DIAL_TONE_HIGH    440.0    // Hz
#define EXCHANGE_RINGING_LOW       440.0    // Hz  
#define EXCHANGE_RINGING_HIGH      480.0    // Hz
#define EXCHANGE_BUSY_LOW          480.0    // Hz
#define EXCHANGE_BUSY_HIGH         620.0    // Hz
#define EXCHANGE_REORDER_LOW       480.0    // Hz
#define EXCHANGE_REORDER_HIGH      620.0    // Hz

// Error tone frequencies (custom implementation)
#define EXCHANGE_ERROR_TONE1       913.8    // Hz
#define EXCHANGE_ERROR_TONE2      1428.5    // Hz  
#define EXCHANGE_ERROR_TONE3      1776.7    // Hz

// Timing constants (milliseconds)
#define EXCHANGE_RINGING_ON_TIME   2000     // 2 seconds on
#define EXCHANGE_RINGING_OFF_TIME  4000     // 4 seconds off
#define EXCHANGE_BUSY_ON_TIME       500     // 0.5 seconds on
#define EXCHANGE_BUSY_OFF_TIME      500     // 0.5 seconds off
#define EXCHANGE_REORDER_ON_TIME    250     // 0.25 seconds on
#define EXCHANGE_REORDER_OFF_TIME   250     // 0.25 seconds off
#define EXCHANGE_DIAL_ON_TIME     30000     // 30 seconds on
#define EXCHANGE_DIAL_OFF_TIME     2000     // 2 seconds off

// Error tone timing (custom implementation)
#define EXCHANGE_ERROR_TONE1_TIME   380     // ms
#define EXCHANGE_ERROR_TONE2_TIME   276     // ms
#define EXCHANGE_ERROR_TONE3_TIME   380     // ms
#define EXCHANGE_ERROR_SILENCE_TIME 2000    // ms

/**
 * SimExchange - Telephone Exchange Simulator
 * 
 * Simulates authentic North American telephone exchange signals including:
 * - Dial tone (350 + 440 Hz)
 * - Ringing tone (440 + 480 Hz) 
 * - Busy signal (480 + 620 Hz)
 * - Reorder signal (480 + 620 Hz, faster cadence)
 * - Error tone (3-tone sequence)
 * 
 * Uses dual wave generators for authentic dual-tone telephony signals.
 * Provides realistic timing patterns for each signal type.
 * 
 * This serves as the flagship station class for FluxTele, equivalent to
 * how SimStation serves FluxTune for amateur radio simulation.
 * 
 * INHERITANCE CONTRACT: Inherits from SimTransmitter which provides
 * Realization base class for FluxTune memory optimization.
 */
class SimExchange : public SimTransmitter
{
public:
    SimExchange(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq, 
                ExchangeSignalType signal_type = EXCHANGE_DIAL_TONE);
    
    virtual bool begin(unsigned long time);
    virtual bool update(Mode *mode);
    virtual bool step(unsigned long time);
    virtual void end();
    virtual void randomize() override;  // Override to randomize signal type
    
    void realize();
    
    // Control methods
    void set_signal_type(ExchangeSignalType signal_type);
    ExchangeSignalType get_signal_type() const { return _signal_type; }
    
    // Telephony-specific methods
    void start_ringing();
    void start_busy_signal();
    void start_dial_tone();
    void start_reorder_signal();
    void start_error_tone();
    void stop_all_signals();
    
    // Debug methods
    void debug_print_signal_info() const;

private:
    ExchangeSignalType _signal_type;
    SignalMeter *_signal_meter;
    
    // Dual generator support for authentic dual-tone telephony
    int _realizer_b;                // Second wave generator realizer ID
    bool _dual_generator_mode;      // True if using dual generators
    
    // State machine for complex signals
    unsigned long _last_state_change;
    bool _tone_active;              // True during ON phase, false during OFF phase
    int _error_tone_step;           // For error tone sequence (0-3)
    
    // Current frequencies for both generators
    float _current_freq_a;          // Primary generator frequency
    float _current_freq_b;          // Secondary generator frequency
    
    // Helper methods
    bool acquire_second_generator();
    void release_second_generator();
    void set_single_tone(float frequency);
    void set_dual_tone(float freq_a, float freq_b);
    
    // Signal realization methods
    void realize_dial_tone(unsigned long current_time, WaveGen *wavegen);
    void realize_ringing(unsigned long current_time, WaveGen *wavegen);
    void realize_busy(unsigned long current_time, WaveGen *wavegen);
    void realize_reorder(unsigned long current_time, WaveGen *wavegen);
    void realize_error_tone(unsigned long current_time, WaveGen *wavegen);
};

#endif // __SIM_EXCHANGE_H__
