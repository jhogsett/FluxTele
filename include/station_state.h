
#ifndef __STATION_STATE_H__
#define __STATION_STATE_H__

// Station states for dynamic station management
enum StationState {
    DORMANT,     // No frequency assigned, minimal memory usage
    ACTIVE,      // Frequency assigned, tracking VFO proximity  
    AUDIBLE,     // Active + has AD9833 generator assigned
    SILENT       // Active but no AD9833 (>4 stations in range)
};

#endif // __STATION_STATE_H__
