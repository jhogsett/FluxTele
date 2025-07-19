// Field Day Configuration - Must be defined before including sim_station.h
#include "station_config.h"

// Temporary debug output for resource testing - disable for production to save memory
// #define DEBUG_STATION_RESOURCES

#include "vfo.h"
#include "wavegen.h"
#include "wave_gen_pool.h"
#include "sim_telco.h"
#include "signal_meter.h"

#define WAIT_SECONDS2 4

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
    _cycles_until_qsy = 30 + (random(30));   // 3-8 cycles before frustration (realistic)

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
        wavegen_a->set_frequency(SPACE_FREQUENCY2, false);
    }

    int realizer_c = get_realizer(realizer_index++);
    if(realizer_c != -1) {
        WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);
        wavegen_c->set_frequency(SPACE_FREQUENCY2, false);
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
                // Operator gets frustrated, QSYs to new frequency
                apply_operator_frustration_drift();
                // Reset frustration counter for next QSY
                _cycles_completed = 0;
                _cycles_until_qsy = 30 + (random(30));   // 3-8 cycles before next frustration
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

void SimTelco::apply_operator_frustration_drift()
{
    // Move to a new frequency as if a whole new operator is on the air
    // Realistic amateur radio operator frequency adjustment
    // ±250 Hz - keep nearby within listening range
    const float DRIFT_RANGE = 250.0f;

    float drift = ((float)random(0, (long)(2.0f * DRIFT_RANGE * 100))) / 100.0f - DRIFT_RANGE;

    // Apply drift to the shared frequency
    _fixed_freq = _fixed_freq + drift;

    // Immediately update the wave generator frequency
    force_frequency_update();
}

void SimTelco::randomize()
{
    // Re-randomize station properties for realistic relocation behavior
    
    // Reset cycle counters to make station behavior feel fresh
    _cycles_completed = 0;
    
    // Set a new random frustration threshold (cycles until QSY)
    _cycles_until_qsy = random(3, 11);  // 3-10 cycles before getting frustrated
    
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
