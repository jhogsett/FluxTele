#ifndef STATION_CONFIG_H
#define STATION_CONFIG_H

// FluxTele Telephony Configuration
// Choose ONE configuration mode by uncommenting it

// ===== FLUXTELE TELEPHONY CONFIGURATIONS =====
// #define CONFIG_TELEPHONE_EXCHANGE  // Default: Multi-line telephone exchange with DTMF dialing (TODO: Implement)
// #define CONFIG_DTMF_DIALER        // DTMF touch-tone dialing demonstration
// #define CONFIG_RING_CADENCE       // Various telephone ringing patterns
// #define CONFIG_BUSY_SIGNAL        // Standard telephone busy signals
// #define CONFIG_MODEM_SOUNDS       // Dial-up modem handshake simulation

// ===== FLUXTELE PRIMARY CONFIGURATION =====  
// #define CONFIG_MIXED_STATIONS    // Primary FluxTele config: SimExchangeBad + CW stations for telephony testing
// #define CONFIG_DEV_LOW_RAM       // Development: Minimal RAM usage for development work
                                 // SAVES ~191 BYTES RAM: Disables RTTY and Pager stations
                                 // Use this for dynamic station pipelining development

// ===== FLUXTUNE DEMONSTRATION CONFIGURATION =====
// #define CONFIG_MIXED_STATIONS    // Uncomment to restore FluxTune functionality for comparison

// ===== TEST CONFIGURATIONS =====
// #define CONFIG_FOUR_CW          // Four CW/Morse stations for CW testing
// #define CONFIG_FIVE_CW          // Five CW/Morse stations for simulating Field Day traffic
// #define CONFIG_TEN_CW           // 21-station stress test for Nano Every (10 CW + 5 Numbers + 4 RTTY + 2 Pager)
// #define CONFIG_TEST_PERFORMANCE  // Single test station for measuring main loop performance
// #define CONFIG_FILE_PILE_UP     // Five CW/Morse stations simulating Scarborough Reef pile-up (BS77H variations)
// #define CONFIG_FOUR_NUMBERS     // Four Numbers stations for spooky testing
// #define CONFIG_FOUR_PAGER       // Four Pager stations for digital testing
// #define CONFIG_FOUR_RTTY        // Four RTTY stations for RTTY testing
// #define CONFIG_FOUR_JAMMER      // Four Jammer stations for interference testing
// #define CONFIG_PAGER2_TEST      // Single dual-tone pager station for testing dual wave generators
// #define CONFIG_MINIMAL_CW       // Single CW station (minimal memory) - CONFIRMED: Single station causes restarts
#define CONFIG_SIMSTATION2_TEST // Single SimStation2 station for testing duplicate class functionality

// ===== LISTENING PLEASURE CONFIGURATION =====
// #define CONFIG_CW_CLUSTER       // Four CW stations clustered in 40m for listening pleasure
                                   // Frequencies: 7002, 7003.5, 7004.2, 7005.8 kHz
                                   // Speeds: 12, 16, 18, 22 WPM  
                                   // Designed to often overlap in reception for realistic band activity

// ===== MEMORY OPTIMIZATION OPTIONS =====

// Resource Debug - Enable minimal debug output for resource allocation testing
// Only use during resource contention testing - disable for production
// #define DEBUG_WAVE_GEN_POOL  // Uncomment to enable resource debug output
// #define DEBUG_STATION_RESOURCES  // Uncomment to enable station resource debug output

// RTTY Memory Optimization
// For minimal Flash usage, you can disable real Baudot encoding and just generate random bits
// The RTTY simulation will sound authentic but won't transmit actual text
// Saves ~128 bytes of Flash memory from the Baudot lookup table
// #define RTTY_RANDOM_BITS_ONLY  // Uncomment to save Flash memory

// EEPROM Table Storage - Advanced Memory Optimization
// Moves AsyncMorse and AsyncRTTY lookup tables from Flash to EEPROM
// Saves ~164 bytes of Flash at the cost of slower table lookups
// REQUIRES: Run utils/eeprom_table_loader.ino sketch first to program tables
// #define USE_EEPROM_TABLES  // Uncomment to use EEPROM-based table storage

// ===== CONFIGURATION IMPLEMENTATION =====
//
// *** CRITICAL: Array Synchronization Requirements ***
//
// When creating new configurations or modifying existing ones, you MUST update
// the corresponding arrays in src/main.cpp to match the actual station count:
//
// 1. station_pool[SIZE] - Array of station pointers
// 2. realizations[SIZE] - Array of realization pointers (must match station_pool size)
// 3. realization_stats[SIZE] - Status tracking array (around line 590-602)
// 4. RealizationPool constructor SIZE parameter (around line 610-620)
//
// *** RESTART BUG WARNING ***
// Mismatched array sizes cause continuous Arduino restarts!
// Example: Single-station configs need [1], not [3]
//
// ===============================================================================
#ifdef CONFIG_TELEPHONE_EXCHANGE
    // FluxTele: Multi-line telephone exchange simulation
    #define ENABLE_EXCHANGE_STATION // Telephone exchange simulator (SimExchangeBad)
    #define ENABLE_DTMF_STATION     // DTMF dialing station (dual-tone)
    #define ENABLE_RING_STATION     // Ring cadence station  
    #define ENABLE_BUSY_STATION     // Busy signal station
    #define ENABLE_DIAL_TONE_STATION // Dial tone station
#endif

#ifdef CONFIG_DTMF_DIALER
    // FluxTele: DTMF touch-tone dialing demonstration
    #define ENABLE_EXCHANGE_STATION // Telephone exchange simulator (SimExchangeBad)
    #define ENABLE_DTMF_STATION     // DTMF dialing station (dual-tone)
    #define ENABLE_DTMF_MULTI_STATION // Multiple DTMF digits simultaneously
#endif

#ifdef CONFIG_RING_CADENCE
    // FluxTele: Various telephone ringing patterns
    #define ENABLE_EXCHANGE_STATION // Telephone exchange simulator (SimExchangeBad)
    #define ENABLE_RING_STATION     // Standard ring cadence
    #define ENABLE_RING_UK_STATION  // UK ring cadence
    #define ENABLE_RING_OLD_STATION // Old mechanical bell ring
#endif

#ifdef CONFIG_BUSY_SIGNAL
    // FluxTele: Standard telephone busy signals
    #define ENABLE_EXCHANGE_STATION // Telephone exchange simulator (SimExchangeBad)
    #define ENABLE_BUSY_STATION     // Standard busy signal
    #define ENABLE_REORDER_STATION  // Reorder tone (all circuits busy)
    #define ENABLE_FAST_BUSY_STATION // Fast busy signal
#endif

#ifdef CONFIG_MODEM_SOUNDS
    // FluxTele: Dial-up modem handshake simulation
    #define ENABLE_EXCHANGE_STATION // Telephone exchange simulator (SimExchangeBad)
    #define ENABLE_MODEM_DIAL_STATION    // Modem dialing sequence
    #define ENABLE_MODEM_HANDSHAKE_STATION // Modem negotiation sounds
    #define ENABLE_MODEM_CARRIER_STATION   // Modem carrier tones
#endif

// ===== LEGACY FLUXTUNE CONFIGURATIONS =====
#ifdef CONFIG_MIXED_STATIONS
    // Testing: CW + SimRingBad (dual generator telephony baseline)
    #define ENABLE_MORSE_STATION    // Basic CW/Morse station (SimStation)
    // #define ENABLE_EXCHANGE_STATION // Telephone exchange simulator (SimExchangeBad) - NEW!
    #define ENABLE_RING_STATION     // Simple telephone ring simulator (SimRingBad) - NEW! (replaces SimPager2)
    // #define ENABLE_NUMBERS_STATION  // Numbers Station (SimNumbers) - REMOVED for station spacing
    // #define ENABLE_PAGER_STATION    // Pager Station (SimPager) - TEMPORARILY DISABLED for SimPager2 testing
    // #define ENABLE_RTTY_STATION     // RTTY Station (SimRTTY) - REMOVED for station spacing
    // #define ENABLE_PAGER2_STATION   // SimPager2 (dual wave generator) - REPLACED by SimRingBad
    // NOTE: To enable jammer, comment out RING and uncomment below:
    // #define ENABLE_JAMMER_STATION   // Jammer Station (SimJammer) - replaces RING
#endif

#ifdef CONFIG_DEV_LOW_RAM
    // Development: Optimized for low RAM usage during development
    #define ENABLE_MORSE_STATION    // Basic CW/Morse station (SimStation) - essential for testing
    #define ENABLE_NUMBERS_STATION  // Numbers Station (SimNumbers) - moderate RAM usage
    #define ENABLE_TEST_STATION     // Test Station (SimTest) - for loop performance testing
    // #define ENABLE_PAGER_STATION    // Pager Station (SimPager) - disabled to save RAM
    // #define ENABLE_RTTY_STATION     // RTTY Station (SimRTTY) - disabled to save RAM 
    // #define ENABLE_JAMMER_STATION   // Jammer Station (SimJammer) - disabled to save RAM
#endif

#ifdef CONFIG_FOUR_CW
    // Test: Four CW stations with different messages/speeds
    #define ENABLE_FOUR_CW_STATIONS
    #define ENABLE_MORSE_STATION
    // Other stations disabled for focused CW testing
#endif

#ifdef CONFIG_FIVE_CW
    // Test: Five CW stations with different messages/speeds
    #define ENABLE_FOUR_CW_STATIONS
    #define ENABLE_MORSE_STATION
    // Other stations disabled for focused CW testing
#endif

#ifdef CONFIG_TEN_CW
    // Test: Ten CW stations with different messages/speeds for Nano Every memory testing
    #define ENABLE_TEN_CW_STATIONS
    #define ENABLE_MORSE_STATION
    #define ENABLE_PAGER_STATION
    #define ENABLE_RTTY_STATION
    #define ENABLE_NUMBERS_STATION
    // Other stations disabled for focused CW testing
#endif

#ifdef CONFIG_FILE_PILE_UP
    // Test: Five CW stations simulating Scarborough Reef pile-up (BS77H variations)
    #define ENABLE_FOUR_CW_STATIONS
    #define ENABLE_MORSE_STATION
    // Other stations disabled for focused CW testing
#endif

#ifdef CONFIG_FOUR_NUMBERS
    // Test: Four Numbers stations with different frequencies
    #define ENABLE_FOUR_NUMBERS_STATIONS
    #define ENABLE_NUMBERS_STATION
    // Other stations disabled for focused Numbers testing
#endif

#ifdef CONFIG_FOUR_PAGER
    // Test: Four Pager stations
    #define ENABLE_FOUR_PAGER_STATIONS
    #define ENABLE_PAGER_STATION
    // Other stations disabled for focused Pager testing
#endif

#ifdef CONFIG_PAGER2_TEST
    // Test: Single SimPager2 station to test dual wave generator functionality
    #define ENABLE_PAGER2_STATION
    // Other stations disabled for focused dual-generator testing
#endif

#ifdef CONFIG_FOUR_RTTY
    // Test: Four RTTY stations
    #define ENABLE_FOUR_RTTY_STATIONS
    #define ENABLE_RTTY_STATION
    // Other stations disabled for focused RTTY testing
#endif

#ifdef CONFIG_FOUR_JAMMER
    // Test: Four Jammer stations for interference testing
    #define ENABLE_FOUR_JAMMER_STATIONS
    #define ENABLE_JAMMER_STATION
    // Other stations disabled for focused Jammer testing
#endif

#ifdef CONFIG_MINIMAL_CW
    // Minimal: Single CW station for memory testing
    #define ENABLE_MORSE_STATION
    // All other stations disabled
#endif

#ifdef CONFIG_CW_CLUSTER
    // Listening pleasure: Four CW stations clustered in 40m band
    #define ENABLE_CW_CLUSTER_STATIONS
    #define ENABLE_MORSE_STATION
    // Other stations disabled for focused CW listening
#endif

#ifdef CONFIG_SIMSTATION2_TEST
    // Test: Single SimStation2 station for testing duplicate class functionality
    #define ENABLE_SIMSTATION2_TEST
    // All other stations disabled for focused testing
#endif

#ifdef CONFIG_TEST_PERFORMANCE
    // Performance testing: Single test station for measuring main loop speed
    #define ENABLE_TEST_STATION
    // All other stations disabled for clean performance measurement
#endif

#endif // STATION_CONFIG_H
