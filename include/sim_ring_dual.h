#ifndef __SIM_RING_DUAL_H__
#define __SIM_RING_DUAL_H__

#include "sim_dualtone.h"

class SignalMeter; // Forward declaration

// Standard North American ring tone frequencies (440 Hz + 480 Hz)
// These are the fixed offsets that will be added to the station frequency
#define RING_TONE_LOW_OFFSET  440.0   // 440 Hz ring tone frequency offset
#define RING_TONE_HIGH_OFFSET 480.0   // 480 Hz ring tone frequency offset

// Ring cadence timing (North American standard: 2 seconds on, 4 seconds off)
#define RING_ON_DURATION_MS   2000    // 2 seconds of ring tone
#define RING_OFF_DURATION_MS  4000    // 4 seconds of silence

// Ring state machine
enum RingState {
    RING_ACQUIRING,     // Attempting to acquire wave generators
    RING_PLAYING,       // Playing ring tone (2 seconds)
    RING_SILENT,        // Silent pause (4 seconds)
    RING_RETRY          // Failed to acquire generators, waiting to retry
};

#define RING_RETRY_DELAY_MS 1000  // Wait 1 second before retrying generator acquisition

/**
 * SimRingCadence: Telephone Ring Cadence Simulator
 * 
 * Simulates authentic North American telephone ring pattern:
 * - 2 seconds of dual-tone ring (440 Hz + 480 Hz)
 * - 4 seconds of silence
 * - Repeats indefinitely
 * 
 * Uses dual AD9833 wave generators for authentic telephony sound.
 * Handles resource contention gracefully with retry logic.
 */
class SimRing : public SimDualTone
{
public:
    SimRing(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq);
    
    virtual bool begin(unsigned long time) override;
    virtual bool update(Mode *mode) override;
    virtual bool step(unsigned long time) override;
    virtual void end() override;
    
    void realize();
    virtual void randomize() override;  // Re-randomize ring timing variations

    // Set station into retry state (used when wave generator acquisition fails)
    void set_retry_state(unsigned long next_try_time);

private:
    SignalMeter *_signal_meter;
    
    // Ring cadence state machine
    RingState _ring_state;
    unsigned long _state_start_time;    // When current state began
    unsigned long _next_retry_time;     // When to next attempt generator acquisition
    
    // Ring timing variations for realism
    int _ring_on_duration;      // Actual ring on duration (slightly randomized)
    int _ring_off_duration;     // Actual ring off duration (slightly randomized)
    
    // State machine methods
    void enter_acquiring_state(unsigned long time);
    void enter_playing_state(unsigned long time);
    void enter_silent_state(unsigned long time);
    void enter_retry_state(unsigned long time);
    
    // Utility methods
    void start_ring_tone();     // Start playing the dual-tone ring
    void stop_ring_tone();      // Stop ring tone (set to silent)
    void randomize_timing();    // Add slight timing variations for realism
};

#endif
