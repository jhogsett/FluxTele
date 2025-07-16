#ifndef __TELCO_TYPES_H__
#define __TELCO_TYPES_H__

// Telco signal types for different telephony sounds
enum TelcoType {
    TELCO_RING,      // Ring Signal: 440 Hz + 480 Hz, 2s on/4s off
    TELCO_BUSY,      // Busy Signal: 480 Hz + 620 Hz, 0.5s on/0.5s off  
    TELCO_REORDER    // Reorder Signal: 480 Hz + 620 Hz, 0.25s on/0.25s off
};

#endif
