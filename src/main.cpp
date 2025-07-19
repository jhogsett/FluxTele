
#include "station_config.h"

#include <Arduino.h>

#include <Wire.h>

#include <MD_AD9833.h>

#include <Encoder.h>
#include <Adafruit_NeoPixel.h>

#include "displays.h"

#include "hardware.h"


#include "leds.h"

#include "saved_data.h"
#include "seeding.h"

#include "utils.h"

#include "signal_meter.h"

#include "station_config.h"

#include "station_manager.h"

#include "encoder_handler.h"

#include "vfo.h"

#include "vfo_tuner.h"

#include "event_dispatcher.h"

#include "contrast.h"
#include "contrast_handler.h"
#include "bfo.h"
#include "bfo_handler.h"
#include "flashlight.h"
#include "flashlight_handler.h"

#include "wavegen.h"

#ifdef ENABLE_MORSE_STATION
#include "sim_station.h"

#endif

#ifdef ENABLE_SIMDTMF
#include "sim_dtmf.h"
#endif


#ifdef ENABLE_DTMF_TEST
#include "sim_dtmf.h"
#endif

#ifdef ENABLE_DTMF2_TEST
#include "sim_dtmf2.h"
#endif

#ifdef ENABLE_EXCHANGE_BAD_STATION
#include "sim_exchange_bad.h"
#endif

#ifdef ENABLE_RING_BAD_STATION
#include "sim_ring_bad.h"
#endif

#ifdef ENABLE_NUMBERS_STATION
#include "sim_numbers.h"
#endif

#ifdef ENABLE_RTTY_STATION
#include "sim_rtty.h"
#endif

#ifdef ENABLE_PAGER_STATION
#include "sim_pager.h"
#endif

#ifdef ENABLE_PAGER2_STATION
#include "sim_pager2.h"
#endif

#ifdef ENABLE_JAMMER_STATION
#include "sim_jammer.h"
#endif
#ifdef ENABLE_TEST_STATION
#include "sim_test.h"
#endif

#include "wave_gen_pool.h"

#ifdef USE_EEPROM_TABLES
#include "eeprom_tables.h"
#endif

#include "signal_meter.h"

#include "telco_types.h"
#include "sim_telco.h"
#include "sim_dtmf.h"

// ============================================================================
// BRANDING MODE FOR PRODUCT PHOTOGRAPHY
// Comment out this #define to disable branding mode and save Flash memory
// ============================================================================
#define ENABLE_BRANDING_MODE  // OPTIMIZATION: Disabled by default to save Flash

// Create an ledStrip object and specify the pin it will use.
// Now using Adafruit NeoPixel for both platforms
// PololuLedStrip<12> ledStrip;

#define CLKA 3
#define DTA 2
#define SWA 4

#define CLKB 6
#define DTB 5
#define SWB 7

#define PULSES_PER_DETENT 2

// Display handling
// show a display string for 700ms before beginning scrolling for ease of reading
#define DISPLAY_SHOW_TIME 800  // Restored to original value
// scroll the display every 90ms for ease of reading
#define DISPLAY_SCROLL_TIME 70

// scroll flipped options every 100ms
#define OPTION_FLIP_SCROLL_TIME 100

EncoderHandler encoder_handlerA(0, CLKA, DTA, SWA, PULSES_PER_DETENT);
EncoderHandler encoder_handlerB(1, CLKB, DTB, SWB, PULSES_PER_DETENT);

// Pins for SPI comm with the AD9833 IC
const byte PIN_DATA = 11;  ///< SPI Data pin number
const byte PIN_CLK = 13;  	///< SPI Clock pin number
const byte PIN_FSYNC1 = 8; ///< SPI Load pin number (FSYNC in AD9833 usage)
const byte PIN_FSYNC2 = 14;  ///< SPI Load pin number (FSYNC in AD9833 usage)
const byte PIN_FSYNC3 = 15;  ///< SPI Load pin number (FSYNC in AD9833 usage)
const byte PIN_FSYNC4 = 16;  ///< SPI Load pin number (FSYNC in AD9833 usage)

MD_AD9833 AD1(PIN_DATA, PIN_CLK, PIN_FSYNC1); // Arbitrary SPI pins
MD_AD9833 AD2(PIN_DATA, PIN_CLK, PIN_FSYNC2); // Arbitrary SPI pins
MD_AD9833 AD3(PIN_DATA, PIN_CLK, PIN_FSYNC3); // Arbitrary SPI pins
MD_AD9833 AD4(PIN_DATA, PIN_CLK, PIN_FSYNC4); // Arbitrary SPI pins

WaveGen wavegen1(&AD1);
WaveGen wavegen2(&AD2);
WaveGen wavegen3(&AD3);
WaveGen wavegen4(&AD4);

WaveGen *wavegens[4] = {&wavegen1, &wavegen2, &wavegen3, &wavegen4};
bool realizer_stats[4] = {false, false, false, false};
WaveGenPool wave_gen_pool(wavegens, realizer_stats, 4);

// Signal meter instance
SignalMeter signal_meter;

// ============================================================================
// STATION CONFIGURATION - Conditional compilation based on station_config.h
// ============================================================================
//
// *** CRITICAL ARRAY SYNCHRONIZATION REQUIREMENTS ***
//
// When adding/removing stations or creating new configurations, you MUST update:
//
// 1. station_pool[SIZE] array declaration (match actual station count)
// 2. realizations[SIZE] array declaration (must match station_pool size)  
// 3. realization_stats[SIZE] array in conditional #ifdef block (lines ~590-602)
// 4. RealizationPool constructor parameter (lines ~610-620)
//
// Example: CONFIG_MIXED_STATIONS currently has 3 stations:
//   - station_pool[3] and realizations[3] (defined in config sections below)
//   - realization_stats[3] (defined around line 597)
//   - RealizationPool(..., 3) (defined around line 615)
//
// *** RESTART BUG WARNING ***
// Mismatched array sizes cause continuous Arduino restarts!
// Always verify station count matches ALL array declarations!
//
// ============================================================================

#ifdef CONFIG_SIMDTMF

// TEST: Single SimTelco station for testing duplicate class functionality
SimDTMF cw_station2_test1(&wave_gen_pool, &signal_meter, 555123400L);
SimDTMF cw_station2_test2(&wave_gen_pool, &signal_meter, 867530900L);

SimDualTone *station_pool[2] = {  // Now using SimDualTone base class
    &cw_station2_test1
	,
    &cw_station2_test2
};

Realization *realizations[2] = {  // Only 1 entry for test config
	&cw_station2_test1
	,
	&cw_station2_test2
};
#endif

#ifdef CONFIG_SIMTELCO

// TEST: Single SimTelco station for testing duplicate class functionality
SimTelco cw_station2_test1(&wave_gen_pool, &signal_meter, 55500000L, TelcoType::TELCO_DIALTONE);

SimTelco cw_station2_test2(&wave_gen_pool, &signal_meter, 55501000L, TelcoType::TELCO_DIALTONE);

SimDualTone *station_pool[2] = {  // Now using SimDualTone base class
    &cw_station2_test1
	,
    &cw_station2_test2
};

Realization *realizations[2] = {  // Only 1 entry for test config
	&cw_station2_test1
	,
	&cw_station2_test2
};
#endif

#ifdef CONFIG_ALLTELCO

SimTelco cw_station2_test1(&wave_gen_pool, &signal_meter,  555123400L, TelcoType::TELCO_RINGBACK);
SimDTMF cw_station2_test2(&wave_gen_pool, &signal_meter,   555130000L);
SimTelco cw_station2_test3(&wave_gen_pool, &signal_meter,  555200000L, TelcoType::TELCO_DIALTONE);
SimTelco cw_station2_test4(&wave_gen_pool, &signal_meter,  555250000L, TelcoType::TELCO_DIALTONE);
SimTelco cw_station2_test5(&wave_gen_pool, &signal_meter,  555300000L, TelcoType::TELCO_RINGBACK);
SimTelco cw_station2_test6(&wave_gen_pool, &signal_meter,  555350000L, TelcoType::TELCO_RINGBACK);
SimDTMF cw_station2_test7(&wave_gen_pool, &signal_meter,   555400000L);
SimTelco cw_station2_test8(&wave_gen_pool, &signal_meter,  555450000L, TelcoType::TELCO_BUSY);
SimTelco cw_station2_test9(&wave_gen_pool, &signal_meter,  555500000L, TelcoType::TELCO_REORDER);
SimTelco cw_station2_test10(&wave_gen_pool, &signal_meter,  555550000L, TelcoType::TELCO_REORDER);
SimDualTone *station_pool[10] = {  // Now using SimDualTone base class
    &cw_station2_test1,
    &cw_station2_test2,
    &cw_station2_test3,
    &cw_station2_test4,
    &cw_station2_test5,
    &cw_station2_test6,
    &cw_station2_test7,
    &cw_station2_test8
	,
    &cw_station2_test9,
    &cw_station2_test10
};

Realization *realizations[10] = {  // Only 1 entry for test config
    &cw_station2_test1,
    &cw_station2_test2,
    &cw_station2_test3,
    &cw_station2_test4,
    &cw_station2_test5,
    &cw_station2_test6,
    &cw_station2_test7,
    &cw_station2_test8
	,
    &cw_station2_test9,
    &cw_station2_test10
};
#endif


// ============================================================================
// REALIZATION POOL - Initialize with configured realizations
// ============================================================================

#if defined(CONFIG_SIMDTMF) || defined(CONFIG_SIMTELCO)
bool realization_stats[2] = {false, false};  // Single SimTelco test station
#elif defined(CONFIG_ALLTELCO)
bool realization_stats[10] = {false, false, false, false, false, false, false, false, false, false};  // Single SimTelco test station
#endif

#if defined(CONFIG_SIMDTMF) || defined(CONFIG_SIMTELCO)
RealizationPool realization_pool(realizations, realization_stats, 2);  // *** CRITICAL: Count must match arrays above! ***
#elif defined(CONFIG_ALLTELCO)
RealizationPool realization_pool(realizations, realization_stats, 10);  // *** CRITICAL: Count must match arrays above! ***
#endif

// ============================================================================
// STATION MANAGER - Initialize with shared realizations array (FluxTune optimization)
// Memory savings: Eliminates duplicate station_pool[] array via inheritance casting
// ============================================================================

#if defined(CONFIG_SIMDTMF) || defined(CONFIG_SIMTELCO)
StationManager station_manager(realizations, 2);  // Use optimized constructor with shared array
#elif defined(CONFIG_ALLTELCO)
StationManager station_manager(realizations, 10);  // Use optimized constructor with shared array
#endif

// Timer for periodic exchange signal randomization (authentic telephony behavior)
unsigned long last_exchange_randomization = 0;
const unsigned long EXCHANGE_RANDOMIZE_INTERVAL = 30000;  // 30 seconds between signal changes

// DEBUG: Station count verification (updated for FluxTune shared array optimization)
void debug_station_pool_state() {
    Serial.println("=== SHARED REALIZATIONS DEBUG ===");
    Serial.print("Array size (compile time): ");
    Serial.println(sizeof(realizations) / sizeof(realizations[0]));
    
    int actual_count = 0;
    for(int i = 0; i < (sizeof(realizations) / sizeof(realizations[0])); i++) {
        Serial.print("realizations[");
        Serial.print(i);
        Serial.print("] = ");
        if(realizations[i] != nullptr) {
            Serial.println("VALID");
            actual_count++;
        } else {
            Serial.println("nullptr - CRITICAL BUG!");
        }
    }
    Serial.print("Valid stations: ");
    Serial.println(actual_count);
    Serial.println("=== END STATION DEBUG ===");
}

VFO vfoa("EXC A", 555123400L, 100, &realization_pool);
VFO vfob("EXC B", 867530900L, 100, &realization_pool);
VFO vfoc("EXC C", 123456789L, 100, &realization_pool);

Contrast contrast("Contrast");
BFO bfo("Offset");
Flashlight flashlight("Light");

VFO_Tuner tunera(&vfoa);
VFO_Tuner tunerb(&vfob);
VFO_Tuner tunerc(&vfoc);

ContrastHandler contrast_handler(&contrast);
BFOHandler bfo_handler(&bfo);
FlashlightHandler flashlight_handler(&flashlight);

ModeHandler *handlers1[3] = {&tunera, &tunerb, &tunerc};
ModeHandler *handlers3[3] = {&contrast_handler, &bfo_handler, &flashlight_handler};

EventDispatcher dispatcher1(handlers1, 3);
EventDispatcher dispatcher3(handlers3, 3);

EventDispatcher * dispatcher = &dispatcher1;
int current_dispatcher = 1;

#define APP_SIMRADIO 1
#define APP_SETTINGS 2

void setup_display(){
	Wire.begin();

	const byte display_brightnesses[] = {(unsigned char)option_contrast, (unsigned char)option_contrast};
	display.init(display_brightnesses);
	display.clear();
}

void setup_signal_meter(){
	signal_meter.init();
}

void setup_leds(){
	for(byte i = FIRST_LED; i <= LAST_LED; i++){
		pinMode(i, OUTPUT);
		digitalWrite(i, LOW);
	}
}

void setup_buttons(){
}

void setup(){
	Serial.begin(115200);
	randomizer.randomize();

#ifdef USE_EEPROM_TABLES
	// Initialize EEPROM tables if enabled
	if (!eeprom_tables_init()) {
		Serial.println("WARNING: EEPROM tables not loaded!");
		Serial.println("Run the eeprom_table_loader sketch first.");
		// Continue anyway - tables will fall back to Flash or fail gracefully
	}
#endif

	load_save_data();

	setup_leds();

	setup_display();

	setup_signal_meter();

	AD1.begin();
	AD1.setFrequency((MD_AD9833::channel_t)0, 0.1);
	AD1.setFrequency((MD_AD9833::channel_t)1, 0.1);
	AD1.setMode(MD_AD9833::MODE_SINE);

	AD2.begin();
	AD2.setFrequency((MD_AD9833::channel_t)0, 0.1);
	AD2.setFrequency((MD_AD9833::channel_t)1, 0.1);
	AD2.setMode(MD_AD9833::MODE_SINE);

	AD3.begin();
	AD3.setFrequency((MD_AD9833::channel_t)0, 0.1);
	AD3.setFrequency((MD_AD9833::channel_t)1, 0.1);
	AD3.setMode(MD_AD9833::MODE_SINE);
	AD4.begin();
	AD4.setFrequency((MD_AD9833::channel_t)0, 0.1);
	AD4.setFrequency((MD_AD9833::channel_t)1, 0.1);
	AD4.setMode(MD_AD9833::MODE_SINE);

	// Initialize StationManager with dynamic pipelining
	station_manager.enableDynamicPipelining(true);
	station_manager.setupPipeline(555123400); // Start with VFO A frequency (555.123400 MHz)
	
	// DEBUG: Check for station pool array bounds bug
	debug_station_pool_state();

}

#ifdef ENABLE_BRANDING_MODE
// ============================================================================
// BRANDING MODE - Easter egg for product photography
// Activates when encoder A button is pressed during startup
// Sets signal meter to full strength and lights panel LEDs at max brightness
// ============================================================================
void activate_branding_mode() {
	Adafruit_NeoPixel* led_strip = nullptr;

	// Keep display showing "FluxTune" from previous code - perfect for branding photos!
      // Directly set signal meter LEDs to 4x brightness (bypass dynamic system)
	// rgb_color full_colors[LED_COUNT] = 
#ifdef DEVICE_VARIANT_RED_DISPLAY
// Color values for NeoPixel (red, green, blue ordering)
static const uint32_t BRAND_COLORS[SignalMeter::LED_COUNT] = {
    0x0F0000,   // Red
    0x0F0700,   // Orange
    0x0F0F00,   // Yellow
    0x000F00,   // Green
    0x000F0F,   // Cyan
    0x00000F,   // Blue
    0x07000F    // Purple
};
#else
// Color values for NeoPixel (red, green, blue ordering)
static const uint32_t BRAND_COLORS[SignalMeter::LED_COUNT] = {
    0x000F00,   // Green
    0x000F00,   // Green  
    0x000F00,   // Green
    0x000F00,   // Green
    0x0F0F00,   // Yellow
    0x0F0F00,   // Yellow
    0x0F0000,    // Red
};
#endif

	led_strip = new Adafruit_NeoPixel(SignalMeter::LED_COUNT, SIGNAL_METER_PIN, NEO_GRB + NEO_KHZ800);
	led_strip->begin();
	led_strip->clear();
	led_strip->show();
	
	// Enter infinite loop for photography - device stays in perfect display state
	while(true) {
		for(int i = 0; i < SignalMeter::LED_COUNT; i++){
			uint32_t color = BRAND_COLORS[i];
			// uint8_t r = (color >> 16) & 0xFF;
			// uint8_t g = (color >> 8) & 0xFF;
			// uint8_t b = color & 0xFF;
			// led_strip->setPixelColor(i, led_strip->Color(r, g, b));
			led_strip->setPixelColor(i, color);
		}
		led_strip->show();

		// Keep signal meter LEDs at full brightness (handled by SignalMeter class now)
        // Keep both panel LEDs at 4x maximum brightness
        analogWrite(WHITE_PANEL_LED, (PANEL_LOCK_LED_FULL_BRIGHTNESS * 4) / PANEL_LED_BRIGHTNESS_DIVISOR);
        analogWrite(BLUE_PANEL_LED, (PANEL_LOCK_LED_FULL_BRIGHTNESS * 4) / PANEL_LED_BRIGHTNESS_DIVISOR);
        
        // Small delay to prevent overwhelming the processor
        delay(100);
    }
}
#endif

EventDispatcher * set_application(int application, HT16K33Disp *display){
	EventDispatcher *dispatcher;
	char *title;	switch(application){
		case APP_SIMRADIO:
			dispatcher = &dispatcher1;
			current_dispatcher = APP_SIMRADIO;
			title = (FSTR("SimTelco"));
		break;

		case APP_SETTINGS:
			dispatcher = &dispatcher3;
			current_dispatcher = APP_SETTINGS;
			title = (FSTR("Settings"));
		break;	}
	display->scroll_string(title, DISPLAY_SHOW_TIME, DISPLAY_SCROLL_TIME);

	dispatcher->set_mode(display, 0);
	
	// Force realization update when switching to SimRadio to ensure audio resumes immediately
	if(application == APP_SIMRADIO) {
		dispatcher->update_realization();
	}

	return dispatcher;
}

void purge_events(){
	// Clear all pending encoder events
	while(encoder_handlerA.changed() || encoder_handlerB.changed() || 
	      encoder_handlerA.pressed() || encoder_handlerA.long_pressed() ||
	      encoder_handlerB.pressed() || encoder_handlerB.long_pressed());
}

void loop()
{
	display.scroll_string(FSTR("FLuXTeLE"), DISPLAY_SHOW_TIME, DISPLAY_SCROLL_TIME);

#ifdef ENABLE_BRANDING_MODE
    // BRANDING MODE EASTER EGG - Check if encoder A button is pressed during startup
    // Pin 4 (SWA) goes LOW when button is pressed
    if (digitalRead(SWA) == LOW) {
        activate_branding_mode();  // Never returns - infinite loop for photography
    }
#endif

    unsigned long time = millis();

    // panel_leds.begin(time, LEDHandler::STYLE_PLAIN | LEDHandler::STYLE_BLANKING, DEFAULT_PANEL_LEDS_SHOW_TIME, DEFAULT_PANEL_LEDS_BLANK_TIME);
	
	// ============================================================================
	// INITIALIZE 12-STATION DYNAMIC POOL	// Start stations based on configuration
	// ============================================================================
	
#if defined(CONFIG_SIMDTMF) || defined(CONFIG_SIMTELCO)
	cw_station2_test1.begin(time + random(1000));
	cw_station2_test1.set_station_state(AUDIBLE);
	cw_station2_test2.begin(time + random(2000));
	cw_station2_test2.set_station_state(AUDIBLE);
#endif

#if defined(CONFIG_ALLTELCO)
	cw_station2_test1.begin(time + random(1000));
	cw_station2_test1.set_station_state(AUDIBLE);
	cw_station2_test2.begin(time + random(2000));
	cw_station2_test2.set_station_state(AUDIBLE);
	cw_station2_test3.begin(time + random(3000));
	cw_station2_test3.set_station_state(AUDIBLE);
	cw_station2_test4.begin(time + random(4000));
	cw_station2_test4.set_station_state(AUDIBLE);
	cw_station2_test5.begin(time + random(5000));
	cw_station2_test5.set_station_state(AUDIBLE);
	cw_station2_test6.begin(time + random(6000));
	cw_station2_test6.set_station_state(AUDIBLE);
	cw_station2_test7.begin(time + random(7000));
	cw_station2_test7.set_station_state(AUDIBLE);
	cw_station2_test8.begin(time + random(8000));
	cw_station2_test8.set_station_state(AUDIBLE);
	// cw_station2_test9.begin(time + random(9000));
	// cw_station2_test9.set_station_state(AUDIBLE);
	// cw_station2_test10.begin(time + random(10000));
	// cw_station2_test10.set_station_state(AUDIBLE);
#endif

	set_application(APP_SIMRADIO, &display);

	while(true){
		unsigned long time = millis();
				// Update signal meter decay (capacitor-like discharge)
		signal_meter.update(time);
		
		// Update StationManager with current VFO frequency
		// Only update when in VFO mode (dispatcher1)
		if (dispatcher == &dispatcher1) {
			Mode* current_mode = dispatcher->get_current_mode();
			if (current_mode) {
				// We know this is a VFO since we're in dispatcher1
				// Use static_cast since we've verified the type through dispatcher check
				VFO* current_vfo = static_cast<VFO*>(current_mode);
				station_manager.updateStations(current_vfo->_frequency);
			}
		}
		
		// Periodic exchange signal randomization for realistic telephony behavior
		// --- PANEL LOCK LED OVERRIDE ---
        int lock_brightness = signal_meter.get_panel_led_brightness();
        if (lock_brightness > 0) {
            int pwm = (lock_brightness * PANEL_LOCK_LED_FULL_BRIGHTNESS) / (255 * PANEL_LED_BRIGHTNESS_DIVISOR);
            analogWrite(WHITE_PANEL_LED, pwm); // White LED lock indicator
        } else {
            analogWrite(WHITE_PANEL_LED, 0);
        }        // Comment out the old animation:
		realization_pool.step(time);

		// NOTE: Station step() calls are handled automatically by realization_pool.step()
		// No need for manual step() calls - RealizationPool architecture handles this

		encoder_handlerA.step();
		encoder_handlerB.step();

		// Step non-blocking title display if active
		dispatcher->step_title_display(&display);

		// check for changing dispatchers
		bool pressed = encoder_handlerB.pressed();
		bool long_pressed = encoder_handlerB.long_pressed();
		if(pressed || long_pressed){
			if(pressed){
				// char *title;
				switch(current_dispatcher){
					case 1:
						// 
						dispatcher = set_application(APP_SETTINGS, &display); // Go to Settings
						// current_dispatcher = 2;
						// title = (FSTR("AudioOut"));
						break;
						
					case 2:
						// Clear flashlight mode when leaving settings
						signal_meter.clear_flashlight_mode();
						// 
						dispatcher = set_application(APP_SIMRADIO, &display); // &dispatcher1;
						// current_dispatcher = 1;
						// title = (FSTR("SimRadio"));
						break;
				}

				purge_events();
			}
		}
		// Always check encoder state to keep internal driver logic running
		bool encoderA_changed = encoder_handlerA.changed();
		bool encoderB_changed = encoder_handlerB.changed();
		
		// Process encoder events only when not showing title (to prevent missed events)
		if (!dispatcher->is_showing_title()) {
			if(encoderA_changed){
				#ifdef DEBUG_PIPELINING
				// Minimal tuning debug - only show frequency changes
				Mode* current_mode = dispatcher->get_current_mode();
				if (current_mode && dispatcher == &dispatcher1) {
					VFO* current_vfo = static_cast<VFO*>(current_mode);
					Serial.print("VFO: ");
					Serial.println(current_vfo->_frequency);
				}
				#endif
				
				dispatcher->dispatch_event(&display, ID_ENCODER_TUNING, encoder_handlerA.diff(), 0);
				dispatcher->update_display(&display);
				dispatcher->update_signal_meter(&signal_meter);
				
				// // Test: Add StationManager call in encoder A handling (where the problem occurred)
				// station_manager.updateStations(7000000);
				
				dispatcher->update_realization();
			}

			if(encoderB_changed){
				dispatcher->dispatch_event(&display, ID_ENCODER_MODES, encoder_handlerB.diff(), 0);
				purge_events();  // Clear any noise/overshoot after mode change
				
				// Note: No immediate update_display() call here - let show_title() finish first
				dispatcher->update_realization();
			}
		}
		// Note: If showing title, encoder changes are detected but ignored - 
		// this keeps the encoder driver state machine running properly

		pressed = encoder_handlerA.pressed();
		long_pressed = encoder_handlerA.long_pressed();
		if(pressed || long_pressed){
			dispatcher->dispatch_event(&display, ID_ENCODER_TUNING, pressed, long_pressed);
		}
	}

}
