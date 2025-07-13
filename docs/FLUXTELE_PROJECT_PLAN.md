# FluxTele Project Plan

## Overview
FluxTele is a telecommunications simulation project derived from FluxTune, leveraging the dual wave generator breakthrough achieved with SimPager2.

## Core Concept
- **VFO Display**: Phone numbers (555-0000) instead of frequencies
- **Starting Frequency**: 5550000.0 Hz → displayed as 555-0000
- **Increment**: 1 Hz = 1 phone number (555-0001, 555-0002, etc.)
- **"Lines"**: Telecommunications audio objects (equivalent to FluxTune "stations")

## Technical Foundation
- **Atomic Dual Generator Control**: Proven patterns from SimPager2 success
- **Reference Implementation**: call_sequence.ino with authentic US telephone frequencies
- **Architecture**: Copy FluxTune's proven patterns, adapt for telecommunications

## Phase 1: Foundation
- [ ] Copy FluxTune architecture (after fixing station count bug)
- [ ] Implement phone number display system
- [ ] Adapt wave generator pool for telecommunications
- [ ] Create base "SimLine" class (equivalent to SimTransmitter)

## Phase 2: Core Lines (Dual Generator Applications)
- [ ] **SimDialTone**: 350 + 440 Hz (simplest test case)
- [ ] **SimBusyTone**: 480 + 620 Hz 
- [ ] **SimRingTone**: 480 + 440 Hz (headset ring)
- [ ] **SimDTMF**: Full DTMF matrix with authentic frequencies

## Phase 3: Complex Lines
- [ ] **SimCallSequence**: Complete call with random outcomes
- [ ] **SimOffHook**: 4-generator off-hook tone (1400+2060+2450+2600 Hz)
- [ ] **SimErrorTone**: Sequential error tones

## Frequency Reference (from call_sequence.ino)
### Dual Generator Tones:
- **Dial Tone**: 350 + 440 Hz
- **Busy**: 480 + 620 Hz  
- **Ring**: 480 + 440 Hz
- **Reorder**: 480 + 620 Hz

### DTMF Matrix:
- **Rows**: 697, 770, 852, 941 Hz
- **Columns**: 1209, 1336, 1477, 1633 Hz

### Complex Tones:
- **Off-hook**: 1400 + 2060 + 2450 + 2600 Hz (4 generators)
- **Error**: 913.8 → 1428.5 → 1776.7 Hz (sequential)

## Future Expansion (from call_sequence.ino TODO)
- [ ] Fax machine simulation
- [ ] 300 baud modem / 711 TTY
- [ ] EAS warning tones

## Success Criteria
- [ ] Phone number display working
- [ ] Dual generator dial tone playing
- [ ] "Tuning" through phone number spectrum
- [ ] Dynamic line relocation by line manager
- [ ] Complete call sequence simulation

## Notes
- **Terminology**: "Lines" instead of "stations" for telecommunications context
- **UX**: Phone number paradigm instead of frequency display
- **Clean Separation**: Keep FluxTune focused on radio simulation
