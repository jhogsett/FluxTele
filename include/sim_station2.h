#ifndef __SIM_STATION2_H__
#define __SIM_STATION2_H__

// Wave generator selection for dual generator development
// #define ENABLE_GENERATOR_A  // Enable by default
#define ENABLE_GENERATOR_B  // Enable for dual generator testing

#include "async_morse.h"
#include "sim_transmitter2.h"

class SignalMeter; // Forward declaration

#define SPACE_FREQUENCY2 0.1
#define MESSAGE_BUFFER2 50

// Configurable CQ message format - can be overridden by defining before including this header
#ifndef CQ_MESSAGE_FORMAT2
#define CQ_MESSAGE_FORMAT2 "CQ CQ DE %s %s K    "
#endif

class SimStation2 : public SimTransmitter2
{
public:
    SimStation2(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq, int wpm);
    SimStation2(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq, int wpm, byte fist_quality);
    virtual bool begin(unsigned long time) override;
    
    virtual bool update(Mode *mode) override;
    virtual bool step(unsigned long time) override;

    void realize();
    void apply_wpm_drift();         // Add slight WPM drift for realism
    virtual void randomize() override;  // Re-randomize callsign, WPM, and fist quality
    
    // Set station into retry state (used when initialization fails)
    void set_retry_state(unsigned long next_try_time);

private:
    AsyncMorse _morse;
    bool _changed;
    SignalMeter *_signal_meter;
    char _generated_message[MESSAGE_BUFFER2];     // Generated CQ message with random callsign
    int _stored_wpm;                // Stored WPM from constructor  
    int _base_wpm;                  // Store original WPM for drift calculations
      // Operator frustration frequency drift
    int _cycles_completed;          // Number of complete CQ cycles sent
    int _cycles_until_qsy;          // Random number of cycles before operator gets frustrated and QSYs
    
    // Message repetition state
    bool _in_wait_delay;            // True when waiting between CQ repetitions
    unsigned long _next_cq_time;    // Time to start next CQ cycle

private:
    void generate_random_callsign(char *callsign_buffer, size_t buffer_size);
    void generate_cq_message();
    void apply_operator_frustration_drift();
};

#endif
