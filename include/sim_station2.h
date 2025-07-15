#ifndef __SIM_STATION2_H__
#define __SIM_STATION2_H__

// Wave generator selection for dual generator development
#define ENABLE_GENERATOR_A  // Enable by default
#define ENABLE_GENERATOR_C  // Enable for duplication testing

#include "async_telco.h"
#include "sim_dualtone.h"

class SignalMeter; // Forward declaration

#define SPACE_FREQUENCY2 SILENT_FREQ_DT  // Use the DualTone silent frequency constant

class SimStation2 : public SimDualTone
{
public:
    SimStation2(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq);
    virtual bool begin(unsigned long time) override;
    
    virtual bool update(Mode *mode) override;
    virtual bool step(unsigned long time) override;

    void realize();
    virtual void randomize() override;  // Re-randomize station properties
    
    // Set station into retry state (used when initialization fails)
    void set_retry_state(unsigned long next_try_time);

private:
    AsyncTelco _telco;              // AsyncTelco for ring cadence timing
    SignalMeter *_signal_meter;
    
    // Operator frustration frequency drift
    int _cycles_completed;          // Number of complete on/off cycles sent
    int _cycles_until_qsy;          // Random number of cycles before operator gets frustrated and QSYs
    
    // Message repetition state
    bool _in_wait_delay;            // True when waiting between transmission cycles
    unsigned long _next_cycle_time; // Time to start next transmission cycle

private:
    void apply_operator_frustration_drift();
};

#endif
