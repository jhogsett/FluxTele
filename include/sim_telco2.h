#ifndef __SIM_SimTelco22_H__
#define __SIM_SimTelco22_H__

#include "async_telco.h"
#include "sim_dualtone.h"
#include "telco_types.h"

class SignalMeter; // Forward declaration

#define SPACE_FREQUENCY2 SILENT_FREQ  // Use the DualTone silent frequency constant

class SimTelco2 : public SimDualTone
{
public:
    SimTelco2(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq, TelcoType type);
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
    TelcoType _telco_type;          // Type of telco signal (Ring, Busy, Reorder)
    
    float _frequency_offset_a;      // Primary frequency offset (Hz)
    float _frequency_offset_c;      // Secondary frequency offset (Hz)
    
    // Telephony frequency offset constants
    static const float RINGBACK_FREQ_A;  // 440 Hz for ringback tone
    static const float RINGBACK_FREQ_C;  // 480 Hz for ringback tone  
    static const float BUSY_FREQ_A;      // 480 Hz for busy/reorder signals
    static const float BUSY_FREQ_C;      // 620 Hz for busy/reorder signals
    static const float DIAL_FREQ_A;      // 350 Hz for dial tone
    static const float DIAL_FREQ_C;      // 440 Hz for dial tone
    
    // Legacy aliases for backward compatibility
    static const float RING_FREQ_A;      // Deprecated: use RINGBACK_FREQ_A
    static const float RING_FREQ_C;      // Deprecated: use RINGBACK_FREQ_C
    
    // Operator frustration frequency drift
    int _cycles_completed;          // Number of complete on/off cycles sent
    int _cycles_until_qsy;          // Random number of cycles before operator gets frustrated and QSYs
    
    // Message repetition state
    bool _in_wait_delay;            // True when waiting between transmission cycles
    unsigned long _next_cycle_time; // Time to start next transmission cycle

private:
    void apply_operator_frustration_drift();
    void setFrequencyOffsetsForType();  // Set frequency offsets based on telco type

protected:
    // Override frequency offset methods to use stored values instead of macros
    virtual float getFrequencyOffsetA() const override;
    virtual float getFrequencyOffsetC() const override;
};

#endif
