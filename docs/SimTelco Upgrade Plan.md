## SimTelco Upgrade Plan

### Current State

- Successfully plays a simulated ring tone, 440 Hz and 480 Hz, 2s on 4s off, on device start-up at the frequency of 55.5 MHz
- Uses AsyncTelco to manage state and timing
- Tuning knob successfully alters both tones simulate a radio tuning knob experience of a modulated (double-sideband) signal
- Ring continues to sound normal after the station has reloated itself
- The BFO Offset must be set to 0 Hz to avoid interfering with the two ring frequency offsets (440 Hz and 480 Hz)
- The two ring frequencency offsets are applied current via two macros
    ```
    #define GENERATOR_A_TEST_OFFSET 440.0  // Hz offset for testing dual generator operation
    #define GENERATOR_C_TEST_OFFSET 480.0  // Hz offset for testing dual generator operation
    ```
- They are applied in these methods 
    - `SimDualTone::common_frequency_update()`
    - `SimDualTone::force_frequency_update()`

### Development Goals

- When declaring a SimTelco instance object, be able to specify the type of Telco simulation (Ring, Busy, Reorder)
- The SimTelco instance uses the type to get the proper timing candence from the AsyncTelco class
- It also uses the type to select the proper two offset frequencies to use
    - Ring Signal 440 Hz and 480 Hz
    - Busy and Reorder Signals 480 Hz and 620 Hz
- Two floats are added to SimTelco to hold onto to the frequency offsets. They are used in place of the current GENERATOR_A_TEST_OFFSET and GENERATOR_C_TEST_OFFSET macros.

### Development Steps

Important Note: The changes need to be tested on the device after each step, and if the device is not-functioning, the changes should be reverted, not debugged.

1. Add the _type_ to SimTelco (Ring, Busy, Reorder)
    - There may be examples of similar in _AsyncTelco_
2. Add the new parameter to the existing CONFIG_SIMDTMF configuration
3. Test on the device

4. Add to the class the ability to retrieve the necessary two frequency offsets based on the type
5. Add to the class the two floats needed to hold on to the two frequency offsets
6. Test on the device

7. Replace the use of the two macros GENERATOR_A_TEST_OFFSET and GENERATOR_C_TEST_OFFSET with the use of the two new floats in both places they are used
8. Test on the device

9. Change the type in the CONFIG_SIMDTMF config from "Ring" to "Busy"
10. Test on the device
