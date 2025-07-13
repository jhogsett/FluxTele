#ifndef __SIGNAL_METER_H__
#define __SIGNAL_METER_H__

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#endif

// Signal Meter - 7 WS2812 LEDs showing signal strength
// Uses capacitor-like charging/discharging behavior for realistic analog meter response
//
// CAPACITOR BEHAVIOR:
// - add_charge() sends electrical charge pulses (like current into a capacitor)
// - Accumulator builds up charge from multiple pulses
// - update() applies time-based decay (like capacitor discharge through resistor)
// - Results in smooth, realistic meter response with persistence and decay
//
// USAGE:
// 1. Call add_charge() whenever tuning events occur (e.g., encoder changes)
// 2. Call update() regularly in main loop for time-based decay
// 3. Meter will smoothly charge up during tuning and decay when idle
//
// TUNING TIPS:
// - Increase DECAY_RATE for faster meter fall-off (more responsive)
// - Decrease DEFAULT_CHARGE for slower buildup (less sensitive)
// - Decrease DECAY_INTERVAL for smoother decay (more CPU usage)
//
// S-METER SCALING:
// Uncomment to enable square root scaling for S-meter-like behavior
// This compresses strong signals while maintaining sensitivity for weak signals
#define ENABLE_LOGARITHMIC_S_METER  // TESTING: Enable by default for evaluation
class SignalMeter
{
public:
    static const int LED_COUNT = 7;
    
    SignalMeter();
    
    void init();
    void add_charge(int charge_amount = DEFAULT_CHARGE);    // Add charge pulse (like electrical charge into capacitor)
    void update(unsigned long current_time);    // Update with time-based decay
    void clear();
    
    // Legacy method for backward compatibility (now adds charge instead of setting directly)
    void update_signal_strength(int strength);
    
    // Test accessor
    int get_current_strength() const { return _current_strength; }    // Panel LED lock indicator accessor
    int get_panel_led_brightness() const { return _panel_led_accumulator; }
    void clear_panel_led();
    
    // Flashlight mode control
    void set_flashlight_mode(int brightness);  // Set LEDs to white at specified brightness (0-255)
    void clear_flashlight_mode();              // Return to normal signal meter operation

private:
    void write_leds();
    static const int MAX_ACCUMULATOR = 510;     // Maximum accumulator value (2x LED range for resolution)
    // Panel LED lock indicator parameters
    static const int PANEL_LED_MAX_ACCUMULATOR = 255;
    static const int PANEL_LED_DECAY_RATE = 64;
    
    // TUNABLE PARAMETERS for capacitor behavior:
    static const int DECAY_RATE = 16;            // Accumulator decay per update (higher = faster decay)
    static const unsigned long DECAY_INTERVAL = 50;  // Decay update interval in milliseconds
    static const int DEFAULT_CHARGE = 4;        // Default charge amount per pulse (adjusted for square root scaling)
    
    int _accumulator;                           // Current charge accumulator (0 to MAX_ACCUMULATOR)
    int _current_strength;                      // Current display strength (0-255)
    unsigned long _last_decay_time;             // Last time decay was applied
    
    int _panel_led_accumulator;                 // Accumulator for panel LED lock indicator (0 to PANEL_LED_MAX_ACCUMULATOR)
    
    bool _flashlight_mode;                      // True when in flashlight mode
    int _flashlight_brightness;                 // Brightness level for flashlight mode (0-255)

#ifndef NATIVE_BUILD
    // Use Adafruit NeoPixel for both platforms
    static Adafruit_NeoPixel* _led_strip;
#endif
};

#endif // __SIGNAL_METER_H__
