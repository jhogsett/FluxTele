/**
 * Minimal MD_AD9833 Library - Optimized for FluxTune
 * 
 * This is a stripped-down version of the original MD_AD9833 library,
 * optimized specifically for FluxTune's usage pattern:
 * - Only sine wave output (removes square/triangle wave support)
 * - Removes phase shift support (not used)
 * - Optimizes memory usage for dual-channel frequency switching
 * - Maintains compatibility with existing FluxTune code
 * 
 * Original library: https://github.com/MajicDesigns/MD_AD9833
 * License: LGPL-2.1 (same as original)
 */

#ifndef MD_AD9833_MINIMAL_H
#define MD_AD9833_MINIMAL_H

#include <Arduino.h>

/**
 * Minimal AD9833 controller class optimized for FluxTune
 */
class MD_AD9833
{
public:
  /**
   * Channel enumerated type (maintained for compatibility)
   */
  enum channel_t
  {
    CHAN_0 = 0,
    CHAN_1 = 1,
  };

  /**
   * Output mode - simplified to only what FluxTune uses
   */
  enum mode_t
  {
    MODE_SINE = 0,    // Only sine wave mode supported
  };

  /**
   * Constructor - software SPI interface (as used by FluxTune)
   */
  MD_AD9833(uint8_t dataPin, uint8_t clkPin, uint8_t fsyncPin);

  /**
   * Initialize the AD9833 chip
   */
  void begin(void);

  /**
   * Set frequency for specified channel
   */
  void setFrequency(channel_t channel, float frequency);

  /**
   * Set which channel is active for output
   */
  void setActiveFrequency(channel_t channel);

  /**
   * Set output mode (only MODE_SINE supported)
   */
  void setMode(mode_t mode);

private:
  // Hardware register images - only what we need
  uint16_t  _regCtl;        // control register
  uint32_t  _regFreq[2];    // frequency registers for both channels
  
  // Settings cache - minimized
  uint32_t  _mClk;          // reference clock (25MHz default)
  
  // SPI interface
  uint8_t _dataPin;         // DATA pin
  uint8_t _clkPin;          // CLOCK pin  
  uint8_t _fsyncPin;        // FSYNC pin
  
  // Internal methods
  uint32_t calcFreq(float f);          // Calculate frequency register value
  void spiSend(uint16_t data);         // Send data via SPI
  void writeRegister(uint16_t data);   // Write to AD9833 register
};

#endif // MD_AD9833_MINIMAL_H
