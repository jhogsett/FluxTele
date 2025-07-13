#ifndef __SEEDING_H__
#define __SEEDING_H__

#include <Arduino.h>
#include <random_seed.h>

// LSB (noisiest bit) of an analog read on an unused analog pin
// used to fill an int, one bit at a time, to create the random seed
#define RANDOM_SEED_PIN A7

extern RandomSeed<RANDOM_SEED_PIN> randomizer;

#endif
