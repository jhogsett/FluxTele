#ifndef __SIM_RTTY_H__
#define __SIM_RTTY_H__

#include "sim_transmitter.h"
#include "async_rtty.h"

class SignalMeter; // Forward declaration

#define MARK_FREQ_SHIFT 170.0
#define RTTY_WAIT_SECONDS 6      // Wait time between message rounds
#define RTTY_MARK_TONE_SECONDS 3 // Duration of MARK tone between messages and at round start

class SimRTTY : public SimTransmitter
{
public:
    SimRTTY(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq);    virtual bool begin(unsigned long time) override;
    
    virtual bool update(Mode *mode) override;
    virtual bool step(unsigned long time) override;
    
    void realize();
    
    // Debug method to check state
    bool is_in_wait_delay() const { return _in_wait_delay; }
    
private:
    AsyncRTTY _rtty;
    int _phase;
    SignalMeter *_signal_meter;     // Pointer to signal meter for charge pulses    // Message cycling state
    bool _in_wait_delay;            // True when waiting between messages
    bool _in_round_break;           // True when in long silent period between rounds
    bool _in_initial_mark;          // True when in initial MARK tone at start of round
    unsigned long _next_message_time;  // Time to start next message
    int _message_repeat_count;      // How many times to repeat current message
    int _current_repeat;            // Current repetition number
};

#endif
