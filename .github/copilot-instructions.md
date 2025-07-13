<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->

# FluxTele Copilot Instructions

## Project Overview
FluxTele is an authentic telephone exchange simulator for Arduino Nano Every that generates realistic telephony sounds using multiple AD9833 wave generators. The project is derived from FluxTune and inherits its proven dual wave generator control architecture.

## Key Technical Context

### Hardware Platform
- **Arduino Nano Every** (ATMEGA4809, 48KB Flash, 6KB RAM)
- **4x AD9833 Wave Generator Modules** connected via SPI
- **Rotary Encoder** for VFO tuning and user interaction
- **Optional**: OLED display and NeoPixel LED indicators

### Core Architecture (Inherited from FluxTune)
- **WaveGenPool**: Manages 4 AD9833 wave generators with resource allocation
- **StationManager**: Handles telephony "line" lifecycle and frequency management
- **RealizationPool**: Maps telephone lines to available wave generators
- **Dual Generator Control**: Proven breakthrough allowing simultaneous control of 2 AD9833s

### Critical Code Patterns
1. **Station Count**: Always use `actual_station_count` parameter, NEVER `MAX_STATIONS` constant in loops
2. **Resource Acquisition**: Always check return values from wave generator pool access
3. **Debug Statements**: Use conditional compilation (`#ifdef`), not always-compiled strings
4. **Configuration**: Single `#define` in `station_config.h` controls entire build

### Memory Management
- **Flash Usage**: Currently ~62% (~30KB), optimized for production
- **RAM Usage**: Currently ~25% (~1.5KB), excellent headroom
- **Optimization**: Conditional debug compilation, minimal string literals

## Telephony-Specific Context

### DTMF Implementation
FluxTele uses authentic DTMF frequencies for realistic telephone sounds:
```cpp
// Row frequencies (low)
#define DTMF_ROW_1    697.0     // Rows 1, 2, 3
#define DTMF_ROW_2    770.0     // Rows 4, 5, 6  
#define DTMF_ROW_3    852.0     // Rows 7, 8, 9
#define DTMF_ROW_4    941.0     // Rows *, 0, #

// Column frequencies (high)
#define DTMF_COL_1    1209.0    // Columns 1, 4, 7, *
#define DTMF_COL_2    1336.0    // Columns 2, 5, 8, 0
#define DTMF_COL_3    1477.0    // Columns 3, 6, 9, #
#define DTMF_COL_4    1633.0    // Columns A, B, C, D
```

### Standard Telephony Frequencies
- **Dial Tone**: 350 + 440 Hz continuous
- **Busy Signal**: 480 + 620 Hz (0.5s on/off)
- **Ring Tone**: 440 + 480 Hz (2s on/4s off) 
- **Reorder**: 480 + 620 Hz (0.25s on/off)

### Dual Generator Logic
- **Primary Generator**: Handles first frequency of dual-tone pairs
- **Secondary Generator**: Handles second frequency of dual-tone pairs
- **Resource Safety**: Always verify acquisition before use

## Development Guidelines

### Adding New Telephony Features
1. **Follow SimTransmitter Pattern**: Inherit from base class
2. **Implement Required Methods**: `begin()`, `update()`, `step()`, `end()`
3. **Use Telephony Frequencies**: Build on DTMF foundation
4. **Test Resource Management**: Verify no generator conflicts

### Configuration Management
- **Single Source**: All configurations in `include/station_config.h`
- **Conditional Compilation**: Use `#ifdef` guards for feature sets
- **Array Synchronization**: Ensure station arrays match actual count

### Code Style
- **Descriptive Names**: Use telephony-relevant terminology
- **Resource Safety**: Always check acquisition return values
- **Memory Efficiency**: Minimize string literals and dynamic allocation
- **Arduino Idioms**: Follow Arduino/PlatformIO conventions

## File Structure Guidance

### Core Files
- `src/main.cpp` - Application entry point and setup
- `include/station_config.h` - Telephony configuration selection
- `src/station_manager.cpp` - Call routing and line management (rename from FluxTune)
- `src/wave_gen_pool.cpp` - AD9833 resource management

### Telephony Simulators
- `src/sim_telephone.cpp` - Base telephone simulator classes
- `src/sim_dtmf.cpp` - DTMF tone generation
- `src/sim_ring.cpp` - Ring cadence simulation
- `src/sim_busy.cpp` - Busy signal generation

### Inherited Infrastructure
- `src/wavegen.cpp` - AD9833 low-level control
- `src/displays.cpp` - OLED display management
- `src/leds.cpp` - NeoPixel status indicators

## Build and Debug

### PlatformIO Commands
```bash
pio run                    # Build project
pio run --target upload    # Upload to Arduino
pio device monitor         # Serial monitor
```

### Memory Monitoring
- Watch Flash usage (target <75% for headroom)
- Monitor RAM usage (target <50% for stability)
- Use `pio run --target size` for memory analysis

### Debug Guidelines
- Use conditional compilation for debug output
- Minimize Serial.print statements in production builds
- Test all configurations before committing changes

## Historical Context

FluxTele was derived from FluxTune in July 2025 after resolving critical bugs:
1. **StationManager MAX_STATIONS** array bounds bug (caused continuous restarts)
2. **Flash usage optimization** (removed debug statements)
3. **SimPager2 enhancement** (authentic DTMF frequencies)

The derivation preserved all proven architecture while focusing on telephony applications. Refer to `FLUXTELE_DERIVATION_HANDOFF.md` for complete technical history.

## AI Assistant Guidelines

When working on FluxTele:
1. **Preserve Core Architecture**: Don't break the proven wave generator control
2. **Focus on Telephony**: Build telephony features on existing foundation
3. **Memory Conscious**: Always consider Flash/RAM impact
4. **Test Thoroughly**: Verify no resource conflicts or continuous restarts
5. **Document Changes**: Maintain clear commit messages and documentation

Remember: FluxTele inherits a production-ready, debugged foundation. Build upon it carefully while maintaining the telephony focus.
