// ============================================================================
// EEPROM TABLE LOADER SKETCH
// ============================================================================
// This sketch programs lookup tables into EEPROM for FluxTune memory optimization
// 
// USAGE:
// 1. Upload this sketch to your Arduino/FluxTune device
// 2. Open Serial Monitor (115200 baud)
// 3. The sketch will automatically program the tables and report results
// 4. After successful programming, upload your main FluxTune sketch with USE_EEPROM_TABLES enabled
//
// MEMORY SAVINGS:
// - Morse table: ~36 bytes Flash saved
// - Baudot table: ~128 bytes Flash saved
// - Total: ~164 bytes Flash saved
//
// EEPROM USAGE:
// - Morse table: 36 bytes at address 100-135
// - Baudot table: 128 bytes at address 140-267
// - Magic numbers: 2 bytes at addresses 99, 139
// - Total EEPROM used: 166 bytes (addresses 99-267)

#include <Arduino.h>
#include <EEPROM.h>

// Enable EEPROM programming functions for this utility
#define ENABLE_EEPROM_PROGRAMMING

// Include the EEPROM tables system
#include "eeprom_tables.h"

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);  // Configure LED for blink indicator
    delay(1000);  // Give Serial time to initialize
    
    Serial.println("FluxTune EEPROM Table Loader");
    Serial.println("=============================");
    Serial.println();
    
    // Check current EEPROM state
    Serial.print("Checking current EEPROM state... ");
    if (eeprom_tables_valid()) {
        Serial.println("Tables already loaded and valid!");
        Serial.println("No programming needed.");
    } else {
        Serial.println("Tables not found or invalid.");
        Serial.println("Programming tables to EEPROM...");
        Serial.println();
        
        // Program morse table
        Serial.print("Programming Morse table... ");
        program_morse_table_to_eeprom();
        Serial.println("Done!");
        
        // Program baudot table
        Serial.print("Programming Baudot table... ");
        program_baudot_table_to_eeprom();
        Serial.println("Done!");
        
        // Verify programming
        Serial.println();
        Serial.print("Verifying tables... ");
        if (eeprom_tables_valid()) {
            Serial.println("SUCCESS!");
            Serial.println();
            Serial.println("EEPROM tables successfully programmed!");
            Serial.println("You can now upload your main FluxTune sketch");
            Serial.println("with #define USE_EEPROM_TABLES enabled.");
        } else {
            Serial.println("FAILED!");
            Serial.println("ERROR: Table programming failed. Please try again.");
        }
    }
    
    Serial.println();
    Serial.println("EEPROM Memory Map:");
    Serial.println("  Addresses 0-99:   FluxTune settings (preserved)");
    Serial.println("  Address 99:       Morse table magic number");
    Serial.println("  Addresses 100-135: Morse lookup table (36 bytes)");
    Serial.println("  Address 139:      Baudot table magic number");
    Serial.println("  Addresses 140-267: Baudot lookup table (128 bytes)");
    Serial.println();
    Serial.println("Total EEPROM used: 166 bytes");
    Serial.println("Flash memory saved: ~164 bytes");
    Serial.println();
    Serial.println("Loader complete. You may now upload your main sketch.");
}

void loop() {
    // Blink LED to indicate loader is complete
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
}
