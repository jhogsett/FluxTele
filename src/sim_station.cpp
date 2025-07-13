// Field Day Configuration - Must be defined before including sim_station.h
#include "station_config.h"
#ifdef CONFIG_FIVE_CW
#define CQ_MESSAGE_FORMAT "CQ CQ DE %s %s K    "
#endif
#ifdef CONFIG_FILE_PILE_UP
#define CQ_MESSAGE_FORMAT "BS77H BS77H DE %s %s K"
#endif

// Temporary debug output for resource testing - disable for production to save memory
// #define DEBUG_STATION_RESOURCES

#include "vfo.h"
#include "wavegen.h"
#include "wave_gen_pool.h"
#include "sim_station.h"
#include "signal_meter.h"

#define WAIT_SECONDS 4

// mode is expected to be a derivative of VFO
SimStation::SimStation(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq, int wpm)
    : SimTransmitter(wave_gen_pool, fixed_freq), _signal_meter(signal_meter), _stored_wpm(wpm), _base_wpm(wpm)
{
    // Initialize operator frustration drift tracking
    _cycles_completed = 0;
    _cycles_until_qsy = 3 + (random(6));   // 3-8 cycles before frustration (realistic)

    // Initialize repetition state
    _in_wait_delay = false;
    _next_cq_time = 0;

    // Set default perfect fist quality (0 = mechanical precision)
    _morse.set_fist_quality(0);

    // Generate random callsign and CQ message for this session
    generate_cq_message();
}

SimStation::SimStation(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq, int wpm, byte fist_quality)
    : SimTransmitter(wave_gen_pool, fixed_freq), _signal_meter(signal_meter), _stored_wpm(wpm), _base_wpm(wpm)
{
    // Initialize operator frustration drift tracking    // Initialize operator frustration drift tracking
    _cycles_completed = 0;
    _cycles_until_qsy = 3 + (random(6));   // 3-8 cycles before frustration (realistic)

    // Initialize repetition state
    _in_wait_delay = false;
    _next_cq_time = 0;

    // Set specified fist quality (0 = perfect, 255 = maximum bad fist)
    _morse.set_fist_quality(fist_quality);

    // Generate random callsign and CQ message for this session
    generate_cq_message();
}

bool SimStation::begin(unsigned long time){
    if(!common_begin(time, _fixed_freq)) {
        return false;
    }

    // Check if we have a valid realizer before accessing it
    if(_realizer == -1) {
        return false;
    }

    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
    wavegen->set_frequency(SPACE_FREQUENCY, false);    // Set _enabled and force frequency update with existing _vfo_freq
    // _vfo_freq should retain its value from the previous cycle
    _enabled = true;
    force_frequency_update();
    realize();  // CRITICAL: Set active state for audio output!

    // Start first CQ immediately (after frequencies are set)
    _morse.start_morse(_generated_message, _stored_wpm);
    _in_wait_delay = false;

    return true;
}

void SimStation::realize(){
    if(_realizer == -1) {
        return;  // No WaveGen allocated
    }

    if(!check_frequency_bounds()) {
        return;  // Out of audible range
    }

    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
    wavegen->set_active_frequency(_active);
}

// returns true on successful update
bool SimStation::update(Mode *mode){
    common_frequency_update(mode);

    if(_enabled && _realizer != -1){
        WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
        wavegen->set_frequency(_frequency);
    }

    realize();

    return true;
}

// call periodically to keep realization dynamic
// returns true if it should keep going
bool SimStation::step(unsigned long time){
    // Handle morse code timing
    int morse_state = _morse.step_morse(time);
      switch(morse_state){
    	case STEP_MORSE_TURN_ON:
            _active = true;
            realize();
            send_carrier_charge_pulse(_signal_meter);  // Send charge pulse when carrier turns on
    		break;

    	case STEP_MORSE_LEAVE_ON:
            // Carrier remains on - send another charge pulse
            send_carrier_charge_pulse(_signal_meter);
            break;

    	case STEP_MORSE_TURN_OFF:
            _active = false;
            realize();
            // No charge pulse when carrier turns off
    		break;
       	case STEP_MORSE_MESSAGE_COMPLETE:
            // CQ cycle completed! Check if operator gets frustrated and start wait delay
            _active = false;
            realize();

            _cycles_completed++;
            if(_cycles_completed >= _cycles_until_qsy) {
                // Operator gets frustrated, QSYs to new frequency
                apply_operator_frustration_drift();
                  // Reset frustration counter for next QSY
                _cycles_completed = 0;
                _cycles_until_qsy = 3 + (random(6));   // 3-8 cycles before next frustration
            }

            // DYNAMIC PIPELINING: Free WaveGen at end of message cycle
            // This allows other stations to use the WaveGen during our wait period
            end();

            // Start wait delay before next CQ
            _in_wait_delay = true;
            _next_cq_time = time + (WAIT_SECONDS * 1000);
            break;
    }    // Check if it's time to start next CQ cycle
    if(_in_wait_delay && time >= _next_cq_time) {
        // DYNAMIC PIPELINING: Try to reallocate WaveGen for next message cycle
        if(begin(time)) {  // Only proceed if WaveGen is available
            _in_wait_delay = false;

            // Note: begin() now handles frequency update internally
        } else {
            // WaveGen not available - extend wait period and try again later
            // Add randomization to prevent thundering herd problem
            _next_cq_time = time + 500 + random(1000);     // Try again in 0.5-1.5 seconds
        }
    }

    return true;
}

// Set station into retry state (used when initialization fails)
void SimStation::set_retry_state(unsigned long next_try_time) {
    _in_wait_delay = true;
    _next_cq_time = next_try_time;
}

// Use base class end() method for cleanup
void SimStation::generate_random_callsign(char *callsign_buffer, size_t buffer_size)
{
    // Generate fictional amateur radio callsigns for simulation
    // Uses doubled digits (00, 11, 22, etc.) to avoid generating real callsigns
    // This is like using "555" phone numbers in movies - sounds authentic but can't be real
    // Format: [W/K/N][XX][AAA] where XX = doubled digit (00-99)
    const char *prefixes[] = {"W", "K", "N"};

    int prefix_idx = random(3);
    int digit = random(10);  // 0-9, will be doubled
    int suffix_len = 2 + random(2);  // 2 or 3 letters

    // Ensure we don't overflow the buffer (prefix + 2 digits + suffix + null)
    if (suffix_len > (int)buffer_size - 4) {
        suffix_len = buffer_size - 4;
    }
    if (suffix_len < 0) suffix_len = 0;

    // Use doubled digit to ensure fictional callsign
    snprintf(callsign_buffer, buffer_size, "%s%d%d", prefixes[prefix_idx], digit, digit);
    
    for(int i = 0; i < suffix_len; i++) {
        if (strlen(callsign_buffer) >= buffer_size - 1) break; // Prevent overflow
        char letter[2] = {(char)('A' + random(26)), '\0'};
        strncat(callsign_buffer, letter, buffer_size - strlen(callsign_buffer) - 1);
    }
    
    // Ensure null termination
    callsign_buffer[buffer_size - 1] = '\0';
}

void SimStation::generate_cq_message()
{
    char callsign[12];  // Increased buffer size for safety (e.g., "K99ABCD" + null)
    generate_random_callsign(callsign, sizeof(callsign));

    // Generate CQ message using configurable format with safe sprintf
    snprintf(_generated_message, MESSAGE_BUFFER, CQ_MESSAGE_FORMAT, callsign, callsign);
    
    // Ensure null termination
    _generated_message[MESSAGE_BUFFER - 1] = '\0';
}

void SimStation::apply_operator_frustration_drift()
{
	// Move to a new frequency as if a whole new operator is on the air
    // Formerly: Operator gets frustrated by lack of response and QSYs (changes frequency)
    // Realistic amateur radio operator frequency adjustment
	// ±75 Hz - typical for frustrated amateur
    const float DRIFT_RANGE = 250.0f;  // ±250 Hz - keep nearby within listening range

    float drift = ((float)random(0, (long)(2.0f * DRIFT_RANGE * 100))) / 100.0f - DRIFT_RANGE;

    // Apply drift to the base class frequency
    _fixed_freq = _fixed_freq + drift;
      // ENHANCEMENT: Generate new callsign to simulate a completely different operator
    // This makes it appear that a new station has come on frequency instead of
    // the same operator continuing to call CQ after frequency adjustment
    generate_cq_message();

    // ENHANCEMENT: Apply WPM drift to simulate different operator or mood change
    apply_wpm_drift();

    // Immediately update the wave generator frequency
    force_frequency_update();
}

void SimStation::apply_wpm_drift()
{    // Add slight WPM drift for authentic operator variation
    // Real CW operators vary their sending speed slightly, or different operators take over
    // WPM drift range: ±4 WPM around the original speed (increased for testing)
    const int WPM_DRIFT_RANGE = 4;

    // Use Arduino random() function
    int drift = random(-WPM_DRIFT_RANGE, WPM_DRIFT_RANGE + 1);

    // Apply drift to current WPM, but keep within reasonable bounds (8-25 WPM for CW)
    _stored_wpm = _base_wpm + drift;
    if (_stored_wpm < 8) _stored_wpm = 8;
    if (_stored_wpm > 25) _stored_wpm = 25;
}

void SimStation::randomize()
{
    // Re-randomize station properties for realistic relocation behavior
    
    // Generate a new random callsign
    generate_cq_message();  // This internally calls generate_random_callsign()
    
    // Randomize WPM with a full range for relocated stations (8-25 WPM)
    int new_wpm = random(8, 26);  // 8-25 WPM range
    
    _base_wpm = new_wpm;
    _stored_wpm = new_wpm;
    
    // Randomize fist quality (if AsyncMorse supports it)
    // Note: This would require checking if AsyncMorse has a setFistQuality method
    // For now, fist quality is set during construction and would need AsyncMorse extension
    
    // Reset cycle counters to make station behavior feel fresh
    _cycles_completed = 0;
    
    // Set a new random frustration threshold (cycles until QSY)
    _cycles_until_qsy = random(3, 11);  // 3-10 cycles before getting frustrated
    
    // Reset timing state
    _in_wait_delay = false;
    _next_cq_time = 0;  // Will be set properly on next cycle
}
