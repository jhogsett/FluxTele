#ifndef STATION_MANAGER_H
#define STATION_MANAGER_H

#include "station_config.h"  // CRITICAL: Must include config macros BEFORE using them
// #include "sim_transmitter.h"
#include "sim_dualtone.h"
#include "realization.h"
#include <stdint.h>

// Dynamic MAX_STATIONS based on configuration
#ifdef CONFIG_TEN_CW
#define MAX_STATIONS 21
#elif defined(CONFIG_MIXED_STATIONS)
#define MAX_STATIONS 3  // Updated to match actual station count (CW1 + CW2 + SimPager2)
#elif defined(CONFIG_FIVE_CW) || defined(CONFIG_FIVE_CW_RESOURCE_TEST)
#define MAX_STATIONS 5
#elif defined(CONFIG_FOUR_CW) || defined(CONFIG_FOUR_NUMBERS) || defined(CONFIG_FOUR_PAGER) || defined(CONFIG_FOUR_RTTY) || defined(CONFIG_FOUR_JAMMER) || defined(CONFIG_CW_CLUSTER)
#define MAX_STATIONS 4
#elif defined(CONFIG_DEV_LOW_RAM) || defined(CONFIG_FILE_PILE_UP)
#define MAX_STATIONS 3
#elif defined(CONFIG_MINIMAL_CW) || defined(CONFIG_TEST_PERFORMANCE) || defined(CONFIG_PAGER2_TEST)
#define MAX_STATIONS 1
#elif defined(CONFIG_SIMDTMF)
#define MAX_STATIONS 2
#elif defined(CONFIG_ALLTELCO)
#define MAX_STATIONS 10
#else
#define MAX_STATIONS 4  // Default fallback
#endif

#define MAX_AD9833 4

// Debug control - disabled for production
// #define DEBUG_PIPELINING  // Enable for troubleshooting pipelining issues

// Dynamic pipelining configuration
#define PIPELINE_LOOKAHEAD_RANGE 8000    // 8 kHz ahead/behind VFO - accommodate 7.2 kHz station placement
#define PIPELINE_STATION_SPACING 5000    // Minimum 5 kHz between stations
#define PIPELINE_AUDIBLE_RANGE 5000      // Range where stations become audible
#define PIPELINE_REALLOC_THRESHOLD 6000  // Reallocate when VFO moves 6 kHz (60 steps at 100Hz tuning)
#define PIPELINE_TUNE_DETECT_THRESHOLD 100  // Minimum Hz change to detect tuning activity
#define VFO_TUNING_STEP_SIZE 100         // VFO tuning step size in Hz - stations must align to these increments

class StationManager {
public:
    // Standard constructor: Direct array of SimTransmitter pointers
    // StationManager(SimTransmitter* station_ptrs[], int actual_station_count);
    
    // MEMORY OPTIMIZATION: Share RealizationPool array to eliminate duplicate station arrays
    // REQUIREMENT: All array entries MUST be SimTransmitter-derived objects (checked at runtime)
    // This constructor enables zero-copy sharing of the realizations array between:
    // - RealizationPool (uses as Realization* array)
    // - StationManager (casts to SimTransmitter* array)
    // Memory savings: Eliminates one pointer array per configuration (8-168 bytes depending on station count)
    StationManager(Realization* shared_stations[], int actual_station_count);
    
    void updateStations(uint32_t vfo_freq);
    void allocateAD9833();
    void recycleDormantStations(uint32_t vfo_freq);
    SimDualTone* getStation(int idx);
    int getActiveStationCount() const;
    
    // Dynamic pipelining methods
    void enableDynamicPipelining(bool enable = true);
    void setupPipeline(uint32_t vfo_freq);
    void updatePipeline(uint32_t vfo_freq);
    
    // Runtime configuration methods
    bool isDynamicPipeliningEnabled() const { return pipeline_enabled; }
    bool isPipelinePaused() const { return pipeline_enabled && tuning_direction == 0; }
    int getTuningDirection() const { return tuning_direction; }
    uint32_t getPipelineCenterFreq() const { return pipeline_center_freq; }
    
private:
    SimDualTone* stations[MAX_STATIONS];
    int actual_station_count;  // Number of stations actually configured
    int ad9833_assignment[MAX_AD9833]; // Maps AD9833 channels to station indices
    
    // Dynamic pipelining state
    bool pipeline_enabled;
    uint32_t last_vfo_freq;
    uint32_t pipeline_center_freq;
    int tuning_direction; // -1 = down, 0 = stopped, 1 = up
    unsigned long last_tuning_time; // Last time VFO frequency changed significantly
    
    // Private methods
    void activateStation(int idx, uint32_t freq);
    void deactivateStation(int idx);
    int findDormantStation();
    void reallocateStations(uint32_t vfo_freq);
    void updateStationStates(uint32_t vfo_freq);
    int calculateTuningDirection(uint32_t current_freq, uint32_t last_freq);
    bool canInterruptStation(int station_idx, uint32_t vfo_freq) const;
};

#endif // STATION_MANAGER_H
