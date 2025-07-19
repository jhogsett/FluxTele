// Field Day Configuration - Must be defined before including sim_station.h
#include "station_config.h"

// Temporary debug output for resource testing - disable for production to save memory
// #define DEBUG_STATION_RESOURCES

#include "vfo.h"
#include "wavegen.h"
#include "wave_gen_pool.h"
#include "sim_telco.h"
#include "signal_meter.h"

// Per-TelcoType drift settings for realistic operator behavior
// Minimum cycles before station moves (base persistence)
const int DRIFT_MIN_CYCLES[4] = {
    4,  // TELCO_RINGBACK - ringback signals are moderately persistent
    8,  // TELCO_BUSY - busy signals are moderately persistent  
    12,  // TELCO_REORDER - reorder signals are moderately persistent
    1   // TELCO_DIALTONE - dial tones are moderately persistent
};

// Additional random cycles beyond minimum (creates range)
const int DRIFT_ADDITIONAL_CYCLES[4] = {
    4,  // TELCO_RINGBACK - range: 4-8 cycles (same as before)
    8,  // TELCO_BUSY - range: 4-8 cycles (same as before)
    12,  // TELCO_REORDER - range: 4-8 cycles (same as before)  
    1   // TELCO_DIALTONE - range: 4-8 cycles (same as before)
};

// Telephony frequency offset constants (authentic DTMF frequencies)
const float SimTelco::RINGBACK_FREQ_A = 440.0f;  // 440 Hz for ringback tone
const float SimTelco::RINGBACK_FREQ_C = 480.0f;  // 480 Hz for ringback tone  
const float SimTelco::BUSY_FREQ_A = 480.0f;      // 480 Hz for busy/reorder signals
const float SimTelco::BUSY_FREQ_C = 620.0f;      // 620 Hz for busy/reorder signals
const float SimTelco::DIAL_FREQ_A = 350.0f;      // 350 Hz for dial tone
const float SimTelco::DIAL_FREQ_C = 440.0f;      // 440 Hz for dial tone

// Legacy aliases for backward compatibility
const float SimTelco::RING_FREQ_A = SimTelco::RINGBACK_FREQ_A;
const float SimTelco::RING_FREQ_C = SimTelco::RINGBACK_FREQ_C;

// Helper function to calculate drift cycles based on TelcoType
int calculateDriftCycles(TelcoType type) {
    int type_index = (int)type;  // Convert enum to array index
    if (type_index >= 0 && type_index < 4) {
        return DRIFT_MIN_CYCLES[type_index] + random(DRIFT_ADDITIONAL_CYCLES[type_index]);
    }
    return 4 + random(4);  // Fallback to original behavior
}

// mode is expected to be a derivative of VFO
SimTelco::SimTelco(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq, TelcoType type)
    : SimDualTone(wave_gen_pool, fixed_freq), _signal_meter(signal_meter), _telco_type(type)
{
    // Set frequency offsets based on telco type
    setFrequencyOffsetsForType();
    
    // Configure AsyncTelco timing based on telco type
    _telco.configure_timing(type);
    
    // Initialize operator frustration drift tracking
    _cycles_completed = 0;
    _cycles_until_qsy = calculateDriftCycles(type);  // Per-type drift cycles (realistic telephony behavior)

    // Initialize timing state
    _in_wait_delay = false;
    _next_cycle_time = 0;
}

bool SimTelco::begin(unsigned long time){
    // Attempt to acquire all required realizers atomically
    // The new Realization::begin() handles dual generator coordination automatically
    if(!common_begin(time, _fixed_freq)) {
        return false;  // Failed to acquire all needed wave generators
    }

    // Initialize all acquired wave generators
    int realizer_index = 0;
    
    int realizer_a = get_realizer(realizer_index++);
    if(realizer_a != -1) {
        WaveGen *wavegen_a = _wave_gen_pool->access_realizer(realizer_a);
        wavegen_a->set_frequency(SILENT_FREQ, false);
    }

    int realizer_c = get_realizer(realizer_index++);
    if(realizer_c != -1) {
        WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);
        wavegen_c->set_frequency(SILENT_FREQ, false);
    }

    // Set enabled and force frequency update with existing _vfo_freq
    // _vfo_freq should retain its value from the previous cycle
    _enabled = true;
    force_frequency_update();
    realize();  // CRITICAL: Set active state for audio output!

    // Start AsyncTelco ring cadence (repeating)
    _telco.start_telco_transmission(true);
    _in_wait_delay = false;

    return true;
}

void SimTelco::realize(){
    if(!has_all_realizers()) {
        return;  // No WaveGens allocated
    }

    if(!check_frequency_bounds()) {
        return;  // Out of audible range
    }

    // Set active state for all acquired wave generators
    int realizer_index = 0;

    int realizer_a = get_realizer(realizer_index++);
    if(realizer_a != -1) {
        WaveGen *wavegen_a = _wave_gen_pool->access_realizer(realizer_a);
        wavegen_a->set_active_frequency(_active);
    }

    int realizer_c = get_realizer(realizer_index++);
    if(realizer_c != -1) {
        WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);
        wavegen_c->set_active_frequency(_active);
    }
}

// returns true on successful update
bool SimTelco::update(Mode *mode){
    common_frequency_update(mode);

    if(_enabled && has_all_realizers()){

        // Update frequencies for all acquired wave generators
        int realizer_index = 0;

        int realizer_a = get_realizer(realizer_index++);
        if(realizer_a != -1){
            WaveGen *wavegen_a = _wave_gen_pool->access_realizer(realizer_a);
            wavegen_a->set_frequency(_frequency);
        }

        int realizer_c = get_realizer(realizer_index++);
        if(realizer_c != -1){
            WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);
            wavegen_c->set_frequency(_frequency2);
        }
    }

    realize();
    return true;
}

// call periodically to keep realization dynamic
// returns true if it should keep going
bool SimTelco::step(unsigned long time){
    // Handle ring cadence timing using AsyncTelco
    int telco_state = _telco.step_telco(time);
    
    switch(telco_state) {
        case STEP_TELCO_TURN_ON:
            _active = true;
            realize();
            send_carrier_charge_pulse(_signal_meter);  // Send charge pulse when carrier turns on
            break;
            
        case STEP_TELCO_LEAVE_ON:
            // Carrier remains on - send another charge pulse
            send_carrier_charge_pulse(_signal_meter);
            break;
            
        case STEP_TELCO_TURN_OFF:
            _active = false;
            realize();

            // No charge pulse when carrier turns off
            
            // Count completed cycles for frustration logic (when ring cycle ends)
            _cycles_completed++;
            if(_cycles_completed >= _cycles_until_qsy) {
                // Station cycles to new parameters for dynamic listening experience
                randomize_station();
                // Reset frustration counter for next QSY
                _cycles_completed = 0;
                _cycles_until_qsy = calculateDriftCycles(_telco_type);  // Per-type drift cycles
            }
            break;
            
        case STEP_TELCO_LEAVE_OFF:
            // Carrier remains off - no action needed
            break;
            
        case STEP_TELCO_CHANGE_FREQ:
            // Continue transmitting but change frequency (for dual-tone systems)
            // Ring uses single tone, but this prepares for other telephony sounds
            send_carrier_charge_pulse(_signal_meter);
            break;
    }

    // Check if it's time to start next transmission cycle after a wait period
    if(_in_wait_delay && time >= _next_cycle_time) {
        // DYNAMIC PIPELINING: Try to reallocate WaveGen for next transmission cycle
        if(begin(time)) {  // Only proceed if WaveGen is available
            _in_wait_delay = false;
        } else {
            // WaveGen not available - extend wait period and try again later
            // Add randomization to prevent thundering herd problem
            _next_cycle_time = time + 500 + random(1000);     // Try again in 0.5-1.5 seconds
        }
    }

    return true;
}

// Set station into retry state (used when initialization fails)
void SimTelco::set_retry_state(unsigned long next_try_time) {
    _in_wait_delay = true;
    _next_cycle_time = next_try_time;
}

void SimTelco::randomize_station()
{
#ifdef ENABLE_FREQ_DRIFT
    // Move to a new frequency as if a whole new operator is on the air
    // Realistic amateur radio operator frequency adjustment
    // ±250 Hz - keep nearby within listening range
    const float DRIFT_RANGE = 500.0f;
    const float VFO_STEP = 100.0f;  // Match VFO_TUNING_STEP_SIZE from StationManager

    float drift = ((float)random(0, (long)(2.0f * DRIFT_RANGE * 100))) / 100.0f - DRIFT_RANGE;

    // Apply drift to the shared frequency
    float new_freq = _fixed_freq + drift;

    // USABILITY: Align frequency to VFO tuning step boundaries for precise tuning
    _fixed_freq = ((long)(new_freq / VFO_STEP)) * VFO_STEP;
#endif

    // REALISM: Randomly switch to a different TelcoType (different telephone system)
    // This simulates different operators or telephone exchanges coming on the air
    TelcoType new_types[] = {TELCO_RINGBACK, TELCO_BUSY, TELCO_REORDER, TELCO_DIALTONE};
    _telco_type = new_types[random(4)];  // Randomly pick one of the 4 types
    
    // Update frequency offsets for the new telco type
    setFrequencyOffsetsForType();
    
    // Reconfigure AsyncTelco timing for the new type
    _telco.configure_timing(_telco_type);
    
    // Reset frustration counter with new type-specific cycles
    _cycles_until_qsy = calculateDriftCycles(_telco_type);
    
#ifdef ENABLE_FREQ_DRIFT
    // Immediately update the wave generator frequency
    force_frequency_update();
#endif
    
    // REALISM: Add 3-5 second delay before operator starts transmitting again
    // This simulates the time needed for retuning and getting back on the air
    unsigned long restart_delay = 3000 + random(2000);  // 3-5 seconds
    set_retry_state(millis() + restart_delay);
    
    // Stop current transmission to make the delay effective
    end();
}

void SimTelco::randomize()
{
    // Re-randomize station properties for realistic relocation behavior
    
    // Reset cycle counters to make station behavior feel fresh
    _cycles_completed = 0;
    
    // Set a new random frustration threshold (cycles until QSY)
    _cycles_until_qsy = calculateDriftCycles(_telco_type);  // Per-type drift cycles
    
    // Reset timing state
    _in_wait_delay = false;
    _next_cycle_time = 0;  // Will be set properly on next cycle
}

// Set frequency offsets based on telco type
void SimTelco::setFrequencyOffsetsForType() {
    switch (_telco_type) {
        case TELCO_RINGBACK:
            _frequency_offset_a = RINGBACK_FREQ_A;  // 440 Hz
            _frequency_offset_c = RINGBACK_FREQ_C;  // 480 Hz
            break;
            
        case TELCO_BUSY:
        case TELCO_REORDER:
            _frequency_offset_a = BUSY_FREQ_A;  // 480 Hz
            _frequency_offset_c = BUSY_FREQ_C;  // 620 Hz
            break;
            
        case TELCO_DIALTONE:
            _frequency_offset_a = DIAL_FREQ_A;  // 350 Hz
            _frequency_offset_c = DIAL_FREQ_C;  // 440 Hz
            break;
            
        default:
            // Default to ringback frequencies as fallback
            _frequency_offset_a = RINGBACK_FREQ_A;
            _frequency_offset_c = RINGBACK_FREQ_C;
            break;
    }
}

// Override frequency offset methods to use stored values instead of macros
float SimTelco::getFrequencyOffsetA() const {
    return _frequency_offset_a;
}

float SimTelco::getFrequencyOffsetC() const {
    return _frequency_offset_c;
}
