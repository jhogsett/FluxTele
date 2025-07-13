/**
 * Minimal MD_AD9833 Library Implementation - Optimized for FluxTune
 */

#include "MD_AD9833_Minimal.h"

// AD9833 register definitions (only what we need)
#define CMD_FREQ0    0x4000  // Frequency register 0
#define CMD_FREQ1    0x8000  // Frequency register 1
#define CMD_CONTROL  0x0000  // Control register
#define CMD_B28      0x2000  // 28-bit frequency write
#define CMD_FSELECT  0x0800  // Frequency select bit
#define CMD_RESET    0x0100  // Reset bit

// Default reference clock frequency (25MHz)
#define DEFAULT_MCLK 25000000UL

MD_AD9833::MD_AD9833(uint8_t dataPin, uint8_t clkPin, uint8_t fsyncPin)
  : _dataPin(dataPin), _clkPin(clkPin), _fsyncPin(fsyncPin), _mClk(DEFAULT_MCLK)
{
  _regCtl = CMD_CONTROL | CMD_B28;  // Default control register
  _regFreq[0] = 0;
  _regFreq[1] = 0;
}

void MD_AD9833::begin(void)
{
  // Initialize SPI pins
  pinMode(_dataPin, OUTPUT);
  pinMode(_clkPin, OUTPUT);
  pinMode(_fsyncPin, OUTPUT);
  
  // Set initial states
  digitalWrite(_fsyncPin, HIGH);
  digitalWrite(_clkPin, LOW);
  digitalWrite(_dataPin, LOW);
  
  // Reset AD9833 and configure for sine wave output
  writeRegister(CMD_CONTROL | CMD_RESET | CMD_B28);  // Reset
  writeRegister(CMD_CONTROL | CMD_B28);              // Clear reset, ready for operation
  
  // Set both frequencies to a safe default (1kHz)
  setFrequency(CHAN_0, 1000.0);
  setFrequency(CHAN_1, 1000.0);
  
  // Select channel 0 as default
  setActiveFrequency(CHAN_0);
}

void MD_AD9833::setFrequency(channel_t channel, float frequency)
{
  if (channel > CHAN_1) return;  // Invalid channel
  
  // Calculate 28-bit frequency word
  uint32_t freqWord = calcFreq(frequency);
  _regFreq[channel] = freqWord;
  
  // Send frequency data to AD9833
  uint16_t freqCmd = (channel == CHAN_0) ? CMD_FREQ0 : CMD_FREQ1;
  
  // Send LSB first, then MSB (28-bit transfer)
  writeRegister(freqCmd | (freqWord & 0x3FFF));          // Lower 14 bits
  writeRegister(freqCmd | ((freqWord >> 14) & 0x3FFF));  // Upper 14 bits
}

void MD_AD9833::setActiveFrequency(channel_t channel)
{
  if (channel > CHAN_1) return;  // Invalid channel
  
  // Update control register with frequency select bit
  if (channel == CHAN_1) {
    _regCtl |= CMD_FSELECT;   // Select frequency register 1
  } else {
    _regCtl &= ~CMD_FSELECT;  // Select frequency register 0  
  }
  
  writeRegister(_regCtl);
}

void MD_AD9833::setMode(mode_t mode)
{
  // Only MODE_SINE is supported - this is a no-op for compatibility
  // The AD9833 defaults to sine wave output which is what we want
  (void)mode;  // Suppress unused parameter warning
}

uint32_t MD_AD9833::calcFreq(float f)
{
  // Calculate 28-bit frequency word
  // Formula: FreqReg = (Frequency * 2^28) / MCLK
  return (uint32_t)((f * 268435456.0) / _mClk);  // 268435456 = 2^28
}

void MD_AD9833::spiSend(uint16_t data)
{
  // Software SPI implementation
  for (int i = 15; i >= 0; i--) {
    digitalWrite(_clkPin, LOW);
    digitalWrite(_dataPin, (data >> i) & 1);
    digitalWrite(_clkPin, HIGH);
  }
}

void MD_AD9833::writeRegister(uint16_t data)
{
  digitalWrite(_fsyncPin, LOW);   // Start transaction
  spiSend(data);                  // Send 16-bit data
  digitalWrite(_fsyncPin, HIGH);  // End transaction
}
