## Transition to SimDualTone and SimRing

### Current State

- SimTransmitter and SimStation have been duplicated to `SimTransmitter2` and `SimTelco`
- They, along with `Realization` class, have been modified to support multiple AD9833 wave generators per station
    - In the case of Realization class, it can handle requests from 1 up to the max realizers
    - In the case of SimTransmitter2 there are provisions for using two generators per station
        - `_frequency` and `_frequency2` were added 
    - The code to handle dual generators is guarded by:
        - `#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)`
- On the device both stations can be heard playing CW with dual tones, offset by 100 Hz (`GENERATOR_C_TEST_OFFSET`, a temporary hack for testing-by-listening)
    - importantly, they both respond properly to the tuning knob before and after station relocation
        - meaning their audio frequencies change smoothly while dialing with no clicks/pops/artifacts

### Next Steps - Phase One

1. Transition `SimTransmitter2` to `SimDualTone` to support station types using two wave generators (a common Telco case)
    - this might be just renaming the class as the dual tone support seems solid

### Next Steps - Phase Two (after Phase One is confirmed working in the device)

IMPORTANT NOTE: While working on this phase, we need to make small incremental improvements that are verified on the device before continuing. The last time this transition was attempted, we lost dual-tone support and introduced several old bugs.

2. Transition `SimTelco` to `SimRing` to demonstrate one simple Telco simulation
    - this is a large change:
        - `SimTelco` no longer needs `AsyncMorse`
        - There is an earlier developed `AsyncTelco` class for sequencing typical dual-tone Telco sounds that may have been developed to the point of working properly. 
        - It needs it's own set of frequency offets that need to be applied (similar to the 100 Hz temporary hack does mentioned above). In this case 440 Hz and 480 Hz.
            - Because the audible frequency calculations are occuring in `SimDualTone` (formerly `SimTransmitter2`) it may make sense to support those necessary offsets direct into `SimDualTone` 
            - Maybe when `SimRing` constructs it can specify the two needed tone offsets to it's parent class
        - It needs to track it's own simpler state, including
            - acquiring wave generators and playing the ring signal for two seconds
            - releasing the wave generators and going silent for four seconds
            - repeating the above cycle indefinitely
            - entering a retry mode when wave generators cannot be acquired
        - All other station behavior needs to remain unchanged as it's working


