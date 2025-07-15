# Known Issues - FluxTune Radio Simulator

## Current Status
- **Overall Performance**: Excellent after several days of testing
- **Hardware Variants**: Successfully tested on multiple devices including new Amber/Orange display variant
- **Core Functionality**: Stable and reliable operation

## Outstanding Minor Bugs

### 1. CW Station Message Corruption (RAM-related)
**Symptom**: Occasional letter "A" characters appear after spaces at the end of CW station messages
**Frequency**: Rare, intermittent
**Root Cause Theory**: Stack overflow/corruption due to low RAM conditions (76.2% RAM usage)
**Impact**: Minor audio artifact, does not affect core functionality
**Priority**: Low (cosmetic issue)

**Technical Notes**:
- Most likely occurs during high memory pressure situations
- Could be related to string handling in CW message generation
- May correlate with dynamic pipelining operations that allocate/deallocate stations

### 2. Random CW Letter Transmission
**Symptom**: Single random CW letters heard "out of nowhere" during normal operation
**Frequency**: Very rare, sporadic
**Root Cause Theory**: Combination of stack corruption + station relocation sync issues
**Impact**: Brief audio artifact, no functional impact
**Priority**: Low (rare occurrence)

**Technical Notes**:
- Possibly related to stale station state during dynamic pipelining
- May occur during station reallocation when VFO frequency changes
- Could be a race condition in the realization pool during station transitions

## Analysis Notes

Both issues appear to be related to:
1. **Memory pressure** - Operating at 76.2% RAM usage leaves little headroom
2. **Dynamic systems** - Station pipelining and reallocation create complex state transitions
3. **Timing-sensitive operations** - CW timing and station synchronization

## Future Investigation Priorities

When addressing these issues:
1. **Memory optimization** - Reduce RAM usage to provide more headroom
2. **Station lifecycle** - Review dynamic pipelining for proper cleanup
3. **String handling** - Audit CW message generation for buffer overruns
4. **Synchronization** - Ensure atomic operations during station transitions

## Hardware Validation

**Confirmed Working Configurations**:
- Original device (tested extensively)
- Second device (confirmed working)  
- Third device with Amber/Orange display (newly created, functional)

All variants demonstrate consistent behavior with these minor issues.

---
*Last Updated: July 6, 2025*
*Test Duration: Several days continuous operation*
*RAM Usage: 76.2% (1561/2048 bytes)*
*Flash Usage: 82.5% (25358/30720 bytes)*
