#ifndef __EEPROM_TABLES_H__
#define __EEPROM_TABLES_H__

// ============================================================================
// EEPROM TABLE STORAGE SYSTEM
// ============================================================================
// Moves AsyncMorse and AsyncRTTY lookup tables from Flash to EEPROM
// Saves ~164 bytes of Flash memory at the cost of slower table lookups
// Enable with: #define USE_EEPROM_TABLES

#include <Arduino.h>

// ============================================================================
// EEPROM MEMORY MAP
// ============================================================================
// FluxTune uses EEPROM addresses 0-99 for settings/saved data
// Table storage starts at address 100 to avoid conflicts

#define EEPROM_TABLES_START_ADDR    100

// Table locations
#define EEPROM_MORSE_TABLE_ADDR     (EEPROM_TABLES_START_ADDR + 0)   // Morse data: 36 bytes
#define EEPROM_BAUDOT_TABLE_ADDR    (EEPROM_TABLES_START_ADDR + 40)  // Baudot data: 128 bytes

// Table sizes
#define MORSE_TABLE_SIZE    36   // A-Z (26) + 0-9 (10) 
#define BAUDOT_TABLE_SIZE   128  // Full ASCII table (0-127)

// Magic numbers for validation
#define MORSE_TABLE_MAGIC   0xDA  // "DA" = "DAH" (dash in Morse)
#define BAUDOT_TABLE_MAGIC  0x5F  // 0x5F = 95 decimal, close to "RTTY" concept

// ============================================================================
// EEPROM TABLE ACCESS FUNCTIONS
// ============================================================================

#ifdef USE_EEPROM_TABLES

#include <EEPROM.h>

// Check if tables are properly loaded in EEPROM
bool eeprom_tables_valid();

// Load table data from EEPROM (with validation)
unsigned char eeprom_read_morse_data(int index);
unsigned char eeprom_read_baudot_data(int index);

// Initialize/validate EEPROM tables (call once at startup)
bool eeprom_tables_init();

#else

// When EEPROM tables are disabled, these functions should not be called
// but we provide stubs to avoid compilation errors
inline bool eeprom_tables_valid() { return false; }
inline unsigned char eeprom_read_morse_data(int index) { return 0; }
inline unsigned char eeprom_read_baudot_data(int index) { return 0xFF; }
inline bool eeprom_tables_init() { return false; }

#endif // USE_EEPROM_TABLES

// ============================================================================
// EEPROM TABLE PROGRAMMING (for utility sketch)
// ============================================================================

#ifdef ENABLE_EEPROM_PROGRAMMING

// Original morse data for programming into EEPROM
extern const unsigned char morse_table_data[MORSE_TABLE_SIZE];

// Original baudot data for programming into EEPROM  
extern const unsigned char baudot_table_data[BAUDOT_TABLE_SIZE];

// Functions for programming tables into EEPROM (loader sketch only)
void program_morse_table_to_eeprom();
void program_baudot_table_to_eeprom();
void program_all_tables_to_eeprom();

#endif // ENABLE_EEPROM_PROGRAMMING

#endif // __EEPROM_TABLES_H__
