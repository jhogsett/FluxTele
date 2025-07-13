#ifndef __LED_DRIVER_H__
#define __LED_DRIVER_H__

#ifndef NATIVE_BUILD
#include <Arduino.h>

// Check for Nano Every first
#ifdef ARDUINO_AVR_NANO_EVERY
    #include <Adafruit_NeoPixel.h>
#else
    #include <PololuLedStrip.h>
#endif

// Color structure compatible with both libraries
struct led_color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    
    // Constructor for convenience
    led_color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0) : red(r), green(g), blue(b) {}
};

class LEDDriver {
public:
    LEDDriver(int pin, int count);
    void init();
    void write(led_color* colors, int count);
    void clear();
    
private:
    int _pin;
    int _count;
    
#ifdef ARDUINO_AVR_NANO_EVERY
    // Use Adafruit NeoPixel for Nano Every
    Adafruit_NeoPixel* _strip;
#else
    // Use PololuLedStrip for traditional Arduino boards
    // Note: PololuLedStrip uses template-based pin specification
    // This will be handled in the implementation
    void* _strip_ptr;  // Generic pointer, actual type depends on pin
#endif
};

#endif // NATIVE_BUILD

#endif // __LED_DRIVER_H__
