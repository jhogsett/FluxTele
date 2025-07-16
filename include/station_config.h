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
#define CONFIG_SIMTELCO_TEST    // Single SimTelco station for testing duplicate class functionality
// #define CONFIG_DTMF_TEST        // Single DTMF station for testing digit sequence playback
// #define CONFIG_DTMF2_TEST       // Two SimDTMF2 stations in RINGBACK mode for parallel development testing

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

// Low power mode (disabled by default for realistic Field Day environment)
// #define LOW_POWER_MODE   // Uncomment to enable lower clock speeds and optimize for power

// ===== ADVANCED STATION CONFIGURATIONS =====

// Station configurations for Field Day testing
// These are matched with the CONFIG_ modes above

#ifdef CONFIG_MIXED_STATIONS
    // Mixed stations: CW + Numbers + others
    #define ENABLE_MORSE_STATION        // CW/Morse stations
    #define ENABLE_NUMBERS_STATION      // Numbers stations
    
    // Choose ONE primary station type for balanced testing (2-3 stations total):
    
    // Multi-line telephone exchange (OPTION 1 - Recommended for FluxTele focus)
    // #define ENABLE_EXCHANGE_BAD_STATION     // Telephone exchange with random signals
    
    // Digital stations (OPTION 2 - Good for digital mode testing)
    // #define ENABLE_RTTY_STATION       // RTTY stations
    // #define ENABLE_PAGER_STATION      // Pager stations
    
    // Telephony stations (OPTION 3 - Good for telephony focus)
    #define ENABLE_RING_BAD_STATION   // Telephone ring simulator
    
    // Memory optimization: Disable hungry stations when not needed
    // Only enable if you need the specific functionality
    // #define ENABLE_PAGER2_STATION     // Dual-tone pager (uses 2 wave generators)
    // #define ENABLE_JAMMER_STATION     // Interference testing
    
    // Total recommended: 3 stations maximum for stable operation
    // Current: 2 CW + 1 Ring = 3 stations (perfect)
#endif

#ifdef CONFIG_DEV_LOW_RAM
    // Development stations with minimal RAM usage
    #define ENABLE_MORSE_STATION
    #define ENABLE_NUMBERS_STATION
    #define ENABLE_TEST_STATION
    // Other stations disabled to save ~191 bytes RAM
#endif

#ifdef CONFIG_CW_CLUSTER
    // Listening pleasure: CW cluster
    #define ENABLE_MORSE_STATION
    // All other stations disabled for pure CW experience
#endif

#ifdef CONFIG_FOUR_CW
    // Test: Four CW stations with different speeds
    #define ENABLE_MORSE_STATION
    // All other stations disabled for focused CW testing
#endif

#ifdef CONFIG_FIVE_CW
    // Field Day: Five CW stations with realistic fist qualities and speeds
    #define ENABLE_MORSE_STATION
    // All other stations disabled for focused Field Day CW testing
#endif

#ifdef CONFIG_TEN_CW
    // Stress test: 21 stations for Arduino Nano Every comprehensive testing
    #define ENABLE_MORSE_STATION      // 10 CW stations
    #define ENABLE_NUMBERS_STATION    // 5 Numbers stations  
    #define ENABLE_RTTY_STATION       // 4 RTTY stations
    #define ENABLE_PAGER_STATION      // 2 Pager stations
    // Total: 21 stations competing for 4 wave generators
#endif

#ifdef CONFIG_FIVE_CW_RESOURCE_TEST
    // Resource contention: 5 CW stations competing for 4 wave generators
    #define ENABLE_MORSE_STATION
    // All other stations disabled for focused resource testing
#endif

#ifdef CONFIG_FILE_PILE_UP
    // Pile-up simulation: Multiple CW stations calling same DX
    #define ENABLE_MORSE_STATION
    // All other stations disabled for focused pile-up testing
#endif

#ifdef CONFIG_FOUR_NUMBERS
    // Test: Four Numbers stations with different frequencies and speeds
    #define ENABLE_NUMBERS_STATION
    // All other stations disabled for focused Numbers testing
#endif

#ifdef CONFIG_FOUR_PAGER
    // Test: Four Pager stations for digital mode testing
    #define ENABLE_PAGER_STATION
    // All other stations disabled for focused Pager testing
#endif

#ifdef CONFIG_FOUR_RTTY
    // Test: Four RTTY stations for RTTY mode testing
    #define ENABLE_RTTY_STATION
    // All other stations disabled for focused RTTY testing
#endif

#ifdef CONFIG_FOUR_JAMMER
    // Test: Four Jammer stations for interference testing
    #define ENABLE_JAMMER_STATION
    // All other stations disabled for focused Jammer testing
#endif

#ifdef CONFIG_MINIMAL_CW
    // Minimal: Single CW station for memory testing
    #define ENABLE_MORSE_STATION
    // All other stations disabled
#endif

#ifdef CONFIG_PAGER2_TEST
    // Test: Single dual-tone pager station for testing dual wave generators
    #define ENABLE_PAGER2_STATION
    // All other stations disabled for focused testing
#endif

#ifdef CONFIG_SIMTELCO_TEST
    // Test: Single SimTelco station for testing duplicate class functionality
    #define ENABLE_SIMTELCO_TEST
    // All other stations disabled for focused testing
#endif

#ifdef CONFIG_DTMF_TEST
    // Test: Single DTMF station for testing digit sequence playback
    #define ENABLE_DTMF_TEST
    // All other stations disabled for focused testing
#endif

#ifdef CONFIG_DTMF2_TEST
    // Test: Two SimDTMF2 stations in RINGBACK mode for parallel development testing
    #define ENABLE_DTMF2_TEST
    // All other stations disabled for focused testing
#endif

#ifdef CONFIG_TEST_PERFORMANCE
    // Performance testing: Single test station for measuring main loop speed
    #define ENABLE_TEST_STATION
    // All other stations disabled for clean performance measurement
#endif

#endif // STATION_CONFIG_H
