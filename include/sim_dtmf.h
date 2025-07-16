#ifndef __SIM_DTMF_H__
#define __SIM_DTMF_H__

#include "sim_dualtone.h"
#include "signal_meter.h"
#include "async_dtmf.h"

// DTMF frequency constants (authentic AT&T frequencies)
#define DTMF_ROW_1    697.0f     // Rows 1, 2, 3
#define DTMF_ROW_2    770.0f     // Rows 4, 5, 6  
#define DTMF_ROW_3    852.0f     // Rows 7, 8, 9
#define DTMF_ROW_4    941.0f     // Rows *, 0, #

#define DTMF_COL_1    1209.0f    // Columns 1, 4, 7, *
#define DTMF_COL_2    1336.0f    // Columns 2, 5, 8, 0
#define DTMF_COL_3    1477.0f    // Columns 3, 6, 9, #
#define DTMF_COL_4    1633.0f    // Columns A, B, C, D

class SimDTMF : public SimDualTone 
{
public:
    // Constructor for fixed phone number
    SimDTMF(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq, const char* sequence);
    
    // Constructor for random phone number generation (pass nullptr for sequence)
    SimDTMF(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq);

    bool begin(unsigned long time) override;
    bool update(Mode *mode) override;
    bool step(unsigned long time) override;
    void realize();
    void randomize() override;
    
    // Debug method to display current phone number
    void debug_print_phone_number() const;

private:
    SignalMeter* _signal_meter;
    
    // Store digit sequence for AsyncDTMF
    const char* _digit_sequence;
    
    // Random phone number generation
    bool _use_random_numbers;       // True if generating random phone numbers
    char _generated_number[12];     // Buffer for generated phone number (11 digits + null)
    
    // AsyncDTMF timing manager (similar to AsyncTelco in SimTelco)
    AsyncDTMF _dtmf;
    
    // Current digit frequencies
    float _current_row_freq;
    float _current_col_freq;
    
    // Wait delay management for sequence restarts (matching SimTelco pattern)
    bool _in_wait_delay;            // True when waiting between sequence cycles
    unsigned long _next_cycle_time; // Time to start next sequence cycle
    
    // DTMF frequency lookup tables
    static const float ROW_FREQUENCIES[4];
    static const float COL_FREQUENCIES[4];
    static const int DIGIT_TO_ROW[16];
    static const int DIGIT_TO_COL[16];
    
    // Helper methods
    void set_digit_frequencies(char digit);
    int char_to_digit_index(char c);
    void generate_random_nanp_number();  // Generate random North American phone number
    
    // Override frequency offset methods for DTMF frequencies
    float getFrequencyOffsetA() const override;
    float getFrequencyOffsetC() const override;
};

#endif
