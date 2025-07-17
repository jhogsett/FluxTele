#ifndef __SIM_SimTelco2_H__
#define __SIM_SimTelco2_H__

#include "async_telco.h"
#include "async_dtmf.h"
#include "sim_dualtone.h"
#include "telco_types.h"

// DTMF frequency constants (authentic AT&T frequencies)
#define DTMF_ROW_1    697.0f     // Rows 1, 2, 3
#define DTMF_ROW_2    770.0f     // Rows 4, 5, 6  
#define DTMF_ROW_3    852.0f     // Rows 7, 8, 9
#define DTMF_ROW_4    941.0f     // Rows *, 0, #

#define DTMF_COL_1    1209.0f    // Columns 1, 4, 7, *
#define DTMF_COL_2    1336.0f    // Columns 2, 5, 8, 0
#define DTMF_COL_3    1477.0f    // Columns 3, 6, 9, #
#define DTMF_COL_4    1633.0f    // Columns A, B, C, D

class SignalMeter; // Forward declaration

// #define SPACE_FREQUENCY2 SILENT_FREQ  // Use the DualTone silent frequency constant

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

    bool update2();

private:
    SignalMeter *_signal_meter;

    AsyncTelco _telco;              // AsyncTelco for ring cadence timing
    TelcoType _telco_type;          // Type of telco signal (Ring, Busy, Reorder)
    
    // float _frequency_offset_a;      // Primary frequency offset (Hz)
    // float _frequency_offset_c;      // Secondary frequency offset (Hz)
    
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

    // from SimDTMF
    // Store digit sequence for AsyncDTMF
    const char* _digit_sequence;
    
    // Random phone number generation
    bool _use_random_numbers;       // True if generating random phone numbers
    char _generated_number[12];     // Buffer for generated phone number (11 digits + null)
    
    // AsyncDTMF timing manager (similar to AsyncTelco in SimTelco)
    AsyncDTMF _dtmf;
    
    // Current digit frequencies
    // float _current_row_freq;
    // float _current_col_freq;

    // DTMF frequency lookup tables
    static const float ROW_FREQUENCIES[4];
    static const float COL_FREQUENCIES[4];
    static const int DIGIT_TO_ROW[16];
    static const int DIGIT_TO_COL[16];
    
    // Helper methods
    void set_digit_frequencies(char digit);
    int char_to_digit_index(char c);
    void generate_random_nanp_number();  // Generate random North American phone number
    
private:
    void apply_operator_frustration_drift();
    void setFrequencyOffsetsForType();  // Set frequency offsets based on telco type

protected:
    // Override frequency offset methods to use stored values instead of macros
    virtual float getFrequencyOffsetA() const override;
    virtual float getFrequencyOffsetC() const override;
};

#endif
