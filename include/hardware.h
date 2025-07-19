#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#define NUM_LEDS 2
#define FIRST_LED 9
#define LAST_LED 10

#define NUM_PANEL_LEDS 2
#define FIRST_PANEL_LED 9
#define LAST_PANEL_LED 10
#define WHITE_PANEL_LED 9   // Pin 9: White panel LED
#define BLUE_PANEL_LED 10   // Pin 10: Blue panel LED
#define PANEL_LOCK_LED_FULL_BRIGHTNESS 127

// ============================================================================
// DEVICE VARIANT CONFIGURATION
// Comment/uncomment one of these to match your hardware variant:
// ============================================================================
#define DEVICE_VARIANT_GREEN_DISPLAY    // Original green display version
// #define DEVICE_VARIANT_RED_DISPLAY      // Red display version with dimmer LEDs
// #define DEVICE_VARIANT_AMBER_DISPLAY      // Red display version with dimmer LEDs

// Device-specific scaling factors
#if defined(DEVICE_VARIANT_RED_DISPLAY)
    // Red display variant: LEDs are much brighter, scale down significantly
    #define SIGNAL_METER_BRIGHTNESS_DIVISOR 5      // Divide by 5 for red display
    #define PANEL_LED_BRIGHTNESS_DIVISOR 1         // Divide by 5 for red display
#elif defined(DEVICE_VARIANT_AMBER_DISPLAY)
    // Amber display variant: Brighter than RED
    #define SIGNAL_METER_BRIGHTNESS_DIVISOR 4      // No scaling for green display
    #define PANEL_LED_BRIGHTNESS_DIVISOR 1         // Divide by 2 for green display
#else // Green display variant (default): Original brightness
    #define SIGNAL_METER_BRIGHTNESS_DIVISOR 1      // No scaling for green display
    #define PANEL_LED_BRIGHTNESS_DIVISOR 1         // Divide by 2 for green display
#endif

#endif

#define SIGNAL_METER_PIN 12
