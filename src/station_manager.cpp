#include "station_manager.h"
#include "sim_numbers.h" // Example concrete station type

StationManager::StationManager(SimTransmitter* station_ptrs[MAX_STATIONS]) {
    for (int i = 0; i < MAX_STATIONS; ++i) {
        stations[i] = station_ptrs[i];
        stations[i]->setActive(false);
        stations[i]->set_station_state(DORMANT);
    }
    for (int i = 0; i < MAX_AD9833; ++i) {
        ad9833_assignment[i] = -1;
    }
    
    // Initialize dynamic pipelining state
    pipeline_enabled = false;
    last_vfo_freq = 0;
    pipeline_center_freq = 0;
    tuning_direction = 0;
    last_tuning_time = 0;
}

void StationManager::updateStations(uint32_t vfo_freq) {
    if (pipeline_enabled) {
        updatePipeline(vfo_freq);
    }
    
    updateStationStates(vfo_freq);
    allocateAD9833();
}

void StationManager::allocateAD9833() {
    // Clear current assignments
    for (int i = 0; i < MAX_AD9833; ++i) {
        ad9833_assignment[i] = -1;
    }
    
    // Find active stations and assign AD9833 channels to closest ones
    int assigned_count = 0;
    
    // First pass: assign to stations already in AUDIBLE state to avoid disruption
    for (int i = 0; i < MAX_STATIONS && assigned_count < MAX_AD9833; ++i) {
        if (stations[i]->get_station_state() == AUDIBLE) {
            ad9833_assignment[assigned_count] = i;
            assigned_count++;
        }
    }
    
    // Second pass: assign remaining channels to ACTIVE stations
    for (int i = 0; i < MAX_STATIONS && assigned_count < MAX_AD9833; ++i) {
        if (stations[i]->get_station_state() == ACTIVE) {
            ad9833_assignment[assigned_count] = i;
            stations[i]->set_station_state(AUDIBLE);
            assigned_count++;
        }
    }
    
    // Update station states based on assignments
    for (int i = 0; i < MAX_STATIONS; ++i) {
        bool has_assignment = false;
        for (int j = 0; j < MAX_AD9833; ++j) {
            if (ad9833_assignment[j] == i) {
                has_assignment = true;
                break;
            }
        }
        
        if (stations[i]->get_station_state() == ACTIVE || stations[i]->get_station_state() == AUDIBLE) {
            stations[i]->set_station_state(has_assignment ? AUDIBLE : SILENT);
        }
    }
}

void StationManager::recycleDormantStations(uint32_t vfo_freq) {
    // This method is now part of updatePipeline() - keeping for compatibility
    if (pipeline_enabled) {
        updatePipeline(vfo_freq);
    }
}

void StationManager::activateStation(int idx, uint32_t freq) {
    if (idx >= 0 && idx < MAX_STATIONS) {
        stations[idx]->reinitialize(millis(), freq);
        stations[idx]->setActive(true);
        stations[idx]->set_station_state(ACTIVE);
    }
}

void StationManager::deactivateStation(int idx) {
    if (idx >= 0 && idx < MAX_STATIONS) {
        stations[idx]->setActive(false);
        stations[idx]->set_station_state(DORMANT);
        
        // Release any AD9833 assignment
        for (int i = 0; i < MAX_AD9833; ++i) {
            if (ad9833_assignment[i] == idx) {
                ad9833_assignment[i] = -1;
                break;
            }
        }
    }
}

int StationManager::findDormantStation() {
    for (int i = 0; i < MAX_STATIONS; ++i) {
        if (stations[i]->get_station_state() == DORMANT) return i;
    }
    return -1;
}

void StationManager::enableDynamicPipelining(bool enable) {
    pipeline_enabled = enable;
    if (enable) {
        // Reset pipelining state
        tuning_direction = 0;
        last_vfo_freq = 0;
        pipeline_center_freq = 0;
        last_tuning_time = 0;
    }
}

void StationManager::setupPipeline(uint32_t vfo_freq) {
    if (!pipeline_enabled) return;
    
    // Initial pipeline setup - activate all stations but DON'T change their frequencies
    // Let stations keep their natural configured frequencies from main.cpp
    pipeline_center_freq = vfo_freq;
    last_vfo_freq = vfo_freq;
    last_tuning_time = millis();
    tuning_direction = 0; // Start in stopped state
    
    // Activate all stations with their natural frequencies
    for (int i = 0; i < MAX_STATIONS; ++i) {
        // Start the station with its natural frequency (don't call reinitialize)
        stations[i]->begin(millis());
        stations[i]->setActive(true);
        stations[i]->set_station_state(ACTIVE);
        
        #ifdef DEBUG_PIPELINING
        Serial.print("SETUP S");
        Serial.print(i);
        Serial.print(" at ");
        Serial.println((uint32_t)stations[i]->get_fixed_frequency());
        #endif
    }
    
    #ifdef DEBUG_PIPELINING
    Serial.print("INIT: ");
    Serial.println(vfo_freq);
    #endif
}

void StationManager::updatePipeline(uint32_t vfo_freq) {
    if (!pipeline_enabled) return;
    
    unsigned long current_time = millis();
    
    // Check if VFO frequency has changed significantly
    int32_t freq_change = (int32_t)(vfo_freq - last_vfo_freq);
    bool significant_change = abs(freq_change) >= PIPELINE_TUNE_DETECT_THRESHOLD;
    
    if (significant_change) {
        // Update tuning direction and timing
        int new_direction = (freq_change > 0) ? 1 : -1;
        
        // Update tuning direction - always accept new direction for responsive pipelining
        tuning_direction = new_direction;
        
        last_tuning_time = current_time;
        last_vfo_freq = vfo_freq;
        
        // Update pipeline center frequency with hysteresis
        int32_t center_shift = (int32_t)(vfo_freq - pipeline_center_freq);
        if (abs(center_shift) >= PIPELINE_REALLOC_THRESHOLD) {
            // Reduce time between reallocations for more responsive pipelining
            static unsigned long last_realloc_time = 0;
            if (current_time - last_realloc_time > 200) { // 200ms instead of 500ms
                #ifdef DEBUG_PIPELINING
                Serial.print("CALLING reallocateStations, shift=");
                Serial.println(center_shift);
                #endif
                reallocateStations(vfo_freq);
                pipeline_center_freq = vfo_freq;
                last_realloc_time = current_time;
            }
        }
        
        #ifdef DEBUG_PIPELINING
        Serial.print("PIPE: ");
        Serial.print(vfo_freq);
        Serial.print(" dir=");
        Serial.println(tuning_direction);
        #endif
    }
    else if (current_time - last_tuning_time > 5000) { // 5 second settle time - longer to allow listening
        // User has stopped tuning - pause pipeline updates
        if (tuning_direction != 0) {
            tuning_direction = 0;
            #ifdef DEBUG_PIPELINING
            Serial.println("PAUSE");
            #endif
        }
    }
}

void StationManager::reallocateStations(uint32_t vfo_freq) {
    #ifdef DEBUG_PIPELINING
    Serial.print("reallocate called, dir=");
    Serial.println(tuning_direction);
    #endif
    
    if (tuning_direction == 0) {
        #ifdef DEBUG_PIPELINING
        Serial.println("SKIP: dir=0");
        #endif
        return; // Not tuning - don't move stations
    }
    
    // Build list of stations that need to be moved, sorted by distance from VFO
    struct StationDistance {
        int index;
        uint32_t distance;
        bool can_interrupt;
    };
    
    StationDistance candidates[MAX_STATIONS];
    int candidate_count = 0;
    
    // Find stations that are outside the lookahead range
    for (int i = 0; i < MAX_STATIONS; ++i) {
        uint32_t station_freq = (uint32_t)stations[i]->get_fixed_frequency();
        int32_t distance_from_vfo = (int32_t)(station_freq - vfo_freq);
        uint32_t abs_distance = abs(distance_from_vfo);
        
        #ifdef DEBUG_PIPELINING
        Serial.print("S");
        Serial.print(i);
        Serial.print(": ");
        Serial.print(station_freq);
        Serial.print(" dist=");
        Serial.print(abs_distance);
        Serial.print(" vs ");
        Serial.println(PIPELINE_LOOKAHEAD_RANGE);
        #endif
        
        if (abs_distance > PIPELINE_LOOKAHEAD_RANGE) {
            StationState state = stations[i]->get_station_state();
            
            // Determine if station can be safely interrupted
            bool can_interrupt = false;
            if (state == DORMANT || state == SILENT) {
                // Always safe to interrupt dormant or silent stations
                can_interrupt = true;
            } else if (state == ACTIVE) {
                // Active stations can be interrupted if they're far from audible range
                can_interrupt = (abs_distance > PIPELINE_AUDIBLE_RANGE * 2);
            } else if (state == AUDIBLE) {
                // Audible stations can only be interrupted if they're out of audible range
                can_interrupt = (abs_distance > PIPELINE_AUDIBLE_RANGE);
            }
            
            #ifdef DEBUG_PIPELINING
            Serial.print("  state=");
            Serial.print(state);
            Serial.print(" abs_dist=");
            Serial.print(abs_distance);
            Serial.print(" range=");
            Serial.print(PIPELINE_AUDIBLE_RANGE);
            Serial.print(" can_int=");
            Serial.println(can_interrupt);
            #endif
            
            if (can_interrupt) {
                candidates[candidate_count] = {i, abs_distance, can_interrupt};
                candidate_count++;
            }
        }
    }
    
    #ifdef DEBUG_PIPELINING
    Serial.print("Found ");
    Serial.print(candidate_count);
    Serial.println(" candidates");
    #endif
    
    // Sort candidates by distance (furthest first)
    for (int i = 0; i < candidate_count - 1; ++i) {
        for (int j = i + 1; j < candidate_count; ++j) {
            if (candidates[i].distance < candidates[j].distance) {
                StationDistance temp = candidates[i];
                candidates[i] = candidates[j];
                candidates[j] = temp;
            }
        }
    }
    
    // Reallocate stations starting with the furthest ones
    int stations_moved = 0;
    for (int c = 0; c < candidate_count && stations_moved < MAX_STATIONS - 1; ++c) { // Allow moving almost all stations
        int i = candidates[c].index;
        uint32_t new_freq;
        
        if (tuning_direction > 0) {
            // Tuning up - move stations ahead of VFO (higher frequencies)
            new_freq = vfo_freq + 2000 + (stations_moved * 1000); // 2-6 kHz ahead
        } else {
            // Tuning down - place stations BELOW VFO so they can be dialed into
            // As VFO frequency decreases (tuning down), user will eventually tune into these stations
            // Place them 5.7-7.2 kHz below VFO so they start inaudible but become audible as user tunes down
            new_freq = vfo_freq - 5700 - (stations_moved * 500); // 5.7-7.2 kHz behind (below VFO)
        }
        
        // Ensure we don't go below minimum frequency
        if (new_freq < 100000) new_freq = 100000;
        
        // Recycle the station - it's safe to interrupt since we checked above
        stations[i]->reinitialize(millis(), new_freq);
        
        // Re-randomize station properties to make it feel like a completely new station
        stations[i]->randomize();
        
        stations_moved++;
        
        #ifdef DEBUG_PIPELINING
        Serial.print("MOVE: S");
        Serial.print(i);
        Serial.print(" to ");
        Serial.println(new_freq);
        #endif
    }
}

void StationManager::updateStationStates(uint32_t vfo_freq) {
    // Update station states based on proximity to VFO
    for (int i = 0; i < MAX_STATIONS; ++i) {
        if (!stations[i]->isActive()) continue; // Skip inactive stations
        
        uint32_t station_freq = (uint32_t)stations[i]->get_fixed_frequency();
        int32_t signed_freq_diff = (int32_t)(station_freq - vfo_freq);
        uint32_t abs_freq_diff = abs(signed_freq_diff);
        
        StationState current_state = stations[i]->get_station_state();
        
        if (abs_freq_diff <= PIPELINE_AUDIBLE_RANGE) {
            // Station is close enough to be potentially audible
            if (current_state == DORMANT) {
                stations[i]->set_station_state(ACTIVE);
            }
            // Don't downgrade AUDIBLE or SILENT stations - let allocateAD9833() handle that
        } else {
            // Use asymmetric lookahead ranges based on current tuning direction
            uint32_t effective_lookahead_range;
            
            if (tuning_direction > 0) {
                // Tuning up - use standard range for stations above VFO, smaller for below
                effective_lookahead_range = (signed_freq_diff > 0) ? PIPELINE_LOOKAHEAD_RANGE : PIPELINE_LOOKAHEAD_RANGE / 2;
            } else if (tuning_direction < 0) {
                // Tuning down - use larger range for stations below VFO, smaller for above
                effective_lookahead_range = (signed_freq_diff < 0) ? 8000 : PIPELINE_LOOKAHEAD_RANGE / 2; // 8kHz for stations below when tuning down
            } else {
                // Not tuning - use symmetric range
                effective_lookahead_range = PIPELINE_LOOKAHEAD_RANGE;
            }
            
            if (abs_freq_diff > effective_lookahead_range) {
                // Station is very far away - mark as dormant to save resources
                if (current_state != DORMANT) {
                    stations[i]->set_station_state(DORMANT);
                }
            }
            // Stations between AUDIBLE_RANGE and effective_lookahead_range stay in their current state
            // unless they're DORMANT, in which case they become ACTIVE
            else if (current_state == DORMANT) {
                stations[i]->set_station_state(ACTIVE);
            }
        }
    }
}

int StationManager::calculateTuningDirection(uint32_t current_freq, uint32_t last_freq) {
    int32_t diff = (int32_t)(current_freq - last_freq);
    
    if (diff > 1000) return 1;      // Tuning up
    else if (diff < -1000) return -1;   // Tuning down
    else return 0;                      // Not moving significantly
}

bool StationManager::canInterruptStation(int station_idx, uint32_t vfo_freq) const {
    if (station_idx < 0 || station_idx >= MAX_STATIONS) return false;
    
    uint32_t station_freq = (uint32_t)stations[station_idx]->get_fixed_frequency();
    uint32_t distance = abs((int32_t)(station_freq - vfo_freq));
    StationState state = stations[station_idx]->get_station_state();
    
    // Determine if station can be safely interrupted based on state and distance
    switch (state) {
        case DORMANT:
        case SILENT:
            return true; // Always safe to interrupt
            
        case ACTIVE:
            // Active stations can be interrupted if they're far from audible range
            return (distance > PIPELINE_AUDIBLE_RANGE * 2);
            
        case AUDIBLE:
            // Audible stations can only be interrupted if they're out of audible range
            return (distance > PIPELINE_AUDIBLE_RANGE);
            
        default:
            return false;
    }
}

SimTransmitter* StationManager::getStation(int idx) {
    if (idx >= 0 && idx < MAX_STATIONS) return stations[idx];
    return nullptr;
}

int StationManager::getActiveStationCount() const {
    int count = 0;
    for (int i = 0; i < MAX_STATIONS; ++i) {
        if (stations[i]->isActive()) ++count;
    }
    return count;
}
