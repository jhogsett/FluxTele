#ifndef __REALIZATION_H__
#define __REALIZATION_H__

#include "mode.h"
#include "wave_gen_pool.h"

// handles realization using one or more realizers (wave generators)
// Maximum 4 realizers supported (matching hardware wave generator count)

class Mode;

#define MAX_REALIZERS_PER_STATION 4

class Realization
{
public:
    Realization(WaveGenPool *wave_gen_pool, int station_id = 0, int required_realizers = 1);

    virtual bool update(Mode *mode);

    virtual bool begin(unsigned long time);
    virtual bool step(unsigned long time);
    virtual void end();
    
    // Update station ID for debugging (used by jammer which sets frequency dynamically)
    void set_station_id(int station_id) { _station_id = station_id; }
    
    // Virtual method for wave generator refresh - default does nothing
    virtual void force_wave_generator_refresh() {}
    
    // Access methods for multiple realizers
    int get_realizer(int index = 0) const;  // Get specific realizer (-1 if not acquired or invalid index)
    int get_realizer_count() const { return _required_realizers; }
    bool has_all_realizers() const;  // True if all required realizers are acquired

protected:
    WaveGenPool *_wave_gen_pool;
    int _realizers[MAX_REALIZERS_PER_STATION];  // Array of realizer IDs (-1 = not acquired)
    int _required_realizers;  // Number of realizers this station needs (1-4)
    int _station_id;
    
    // Legacy compatibility - returns first realizer
    int _realizer;  // Deprecated: use get_realizer(0) instead
};

#endif
