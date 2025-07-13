#include "eeprom_tables.h"

// ============================================================================
// ORIGINAL TABLE DATA (for EEPROM programming)
// ============================================================================

// Original morse data from async_morse.cpp - moved here for EEPROM programming
const unsigned char morse_table_data[MORSE_TABLE_SIZE] PROGMEM = {
    0b10100000, // A  .-
    0b00011000, // B  -...
    0b01011000, // C  -.-.
    0b00110000, // D  -..
    0b01000000, // E  .
    0b01001000, // F  ..-.
    0b01110000, // G  --.
    0b00001000, // H  ....
    0b00100000, // I  ..
    0b11101000, // J  .---
    0b10110000, // K  -.-
    0b00101000, // L  .-..
    0b11100000, // M  --
    0b01100000, // N  -.
    0b11110000, // O  ---
    0b01101000, // P  .--.
    0b10111000, // Q  --.-
    0b01010000, // R  .-.
    0b00010000, // S  ...
    0b11000000, // T  -
    0b10010000, // U  ..-
    0b10001000, // V  ...-
    0b11010000, // W  .--
    0b10011000, // X  -..-
    0b11011000, // Y  -.--
    0b00111000, // Z  --..
    0b11111100, // 0  -----
    0b11110100, // 1  .----
    0b11100100, // 2  ..---
    0b11000100, // 3  ...--
    0b10000100, // 4  ....-
    0b00000100, // 5  .....
    0b00001100, // 6  -....
    0b00011100, // 7  --...
    0b00111100, // 8  ---..
    0b01111100  // 9  ----.
};

// Original baudot data from async_rtty.cpp - partial table for reference
const unsigned char baudot_table_data[BAUDOT_TABLE_SIZE] PROGMEM = {
    // ASCII 0-31 (control characters) - most not supported
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0-7
    0xFF, 0xFF, 0x02, 0xFF, 0xFF, 0x08, 0xFF, 0xFF, // 8-15  (LF=0x02, CR=0x08)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 16-23
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 24-31
    
    // ASCII 32-63 (space, punctuation, numbers)
    0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 32-39  (space=0x04)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 40-47
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 48-55  (numbers in figures mode)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 56-63
    
    // ASCII 64-95 (@ and uppercase letters)
    0xFF,                                           // 64 (@)
    0x03, 0x19, 0x0E, 0x09, 0x01, 0x0D, 0x1A,     // 65-71 (A-G)
    0x14, 0x06, 0x0B, 0x0F, 0x12, 0x1C, 0x18,     // 72-78 (H-N)
    0x16, 0x17, 0x0A, 0x05, 0x10, 0x07, 0x1E,     // 79-85 (O-U)
    0x1D, 0x13, 0x1B, 0x11, 0x15,                 // 86-90 (V-Z)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF,                  // 91-95 (punctuation)
    
    // ASCII 96-127 (` and lowercase letters)
    0xFF,                                           // 96 (`)
    0x03, 0x19, 0x0E, 0x09, 0x01, 0x0D, 0x1A,     // 97-103 (a-g) same as uppercase
    0x14, 0x06, 0x0B, 0x0F, 0x12, 0x1C, 0x18,     // 104-110 (h-n)
    0x16, 0x17, 0x0A, 0x05, 0x10, 0x07, 0x1E,     // 111-117 (o-u)
    0x1D, 0x13, 0x1B, 0x11, 0x15,                 // 118-122 (v-z)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF                   // 123-127 (punctuation)
};

#ifdef USE_EEPROM_TABLES

// ============================================================================
// EEPROM ACCESS FUNCTIONS
// ============================================================================

bool eeprom_tables_valid() {
    // Check magic numbers to see if tables are properly loaded
    byte morse_magic = EEPROM.read(EEPROM_MORSE_TABLE_ADDR - 1);
    byte baudot_magic = EEPROM.read(EEPROM_BAUDOT_TABLE_ADDR - 1);
    
    return (morse_magic == MORSE_TABLE_MAGIC && baudot_magic == BAUDOT_TABLE_MAGIC);
}

unsigned char eeprom_read_morse_data(int index) {
    if (index < 0 || index >= MORSE_TABLE_SIZE) {
        return 0;  // Invalid index
    }
    return EEPROM.read(EEPROM_MORSE_TABLE_ADDR + index);
}

unsigned char eeprom_read_baudot_data(int index) {
    if (index < 0 || index >= BAUDOT_TABLE_SIZE) {
        return 0xFF;  // Invalid index
    }
    return EEPROM.read(EEPROM_BAUDOT_TABLE_ADDR + index);
}

bool eeprom_tables_init() {
    // Check if tables are valid
    if (eeprom_tables_valid()) {
        return true;  // Tables already loaded and valid
    }
    
    // Tables not found or invalid - this is expected on first run
    // The user must run the table loader sketch first
    return false;
}

#endif // USE_EEPROM_TABLES

// ============================================================================
// TABLE PROGRAMMING FUNCTIONS (for loader sketch)
// ============================================================================

#ifdef ENABLE_EEPROM_PROGRAMMING

void program_morse_table_to_eeprom() {
    // Write magic number first
    EEPROM.write(EEPROM_MORSE_TABLE_ADDR - 1, MORSE_TABLE_MAGIC);
    
    // Write morse table data
    for (int i = 0; i < MORSE_TABLE_SIZE; i++) {
        byte data = pgm_read_byte(morse_table_data + i);
        EEPROM.write(EEPROM_MORSE_TABLE_ADDR + i, data);
    }
}

void program_baudot_table_to_eeprom() {
    // Write magic number first  
    EEPROM.write(EEPROM_BAUDOT_TABLE_ADDR - 1, BAUDOT_TABLE_MAGIC);
    
    // Write baudot table data
    for (int i = 0; i < BAUDOT_TABLE_SIZE; i++) {
        byte data = pgm_read_byte(baudot_table_data + i);
        EEPROM.write(EEPROM_BAUDOT_TABLE_ADDR + i, data);
    }
}

void program_all_tables_to_eeprom() {
    program_morse_table_to_eeprom();
    program_baudot_table_to_eeprom();
    
    // Optional: Write a global magic number to indicate all tables are loaded
    EEPROM.write(EEPROM_TABLES_START_ADDR - 1, 0xEE);  // "EEPROM Tables Present"
}

#endif // ENABLE_EEPROM_PROGRAMMING
