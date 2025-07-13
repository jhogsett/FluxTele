/* to do

fax machine
300 baud modem / 711 TTY


EAS warning




*/


#include <MD_AD9833.h>
#include <SPI.h>
#include <random_seed.h>

#define RANDOM_SEED_PIN A1            // floating pin for seeding the RNG

// for generating higher-quality random number seeds
static RandomSeed<RANDOM_SEED_PIN> randomizer;

// Pins for SPI comm with the AD9833 IC
const uint8_t PIN_DATA = 11;  ///< SPI Data pin number
const uint8_t PIN_CLK = 13;  	///< SPI Clock pin number
const uint8_t PIN_FSYNC1 = 10; ///< SPI Load pin number (FSYNC in AD9833 usage)
const uint8_t PIN_FSYNC2 = 9;  ///< SPI Load pin number (FSYNC in AD9833 usage)
const uint8_t PIN_FSYNC3 = 8;  ///< SPI Load pin number (FSYNC in AD9833 usage)
const uint8_t PIN_FSYNC4 = 7;  ///< SPI Load pin number (FSYNC in AD9833 usage)

MD_AD9833	AD1(PIN_DATA, PIN_CLK, PIN_FSYNC1); // Arbitrary SPI pins
MD_AD9833	AD2(PIN_DATA, PIN_CLK, PIN_FSYNC2); // Arbitrary SPI pins
MD_AD9833	AD3(PIN_DATA, PIN_CLK, PIN_FSYNC3); // Arbitrary SPI pins
MD_AD9833	AD4(PIN_DATA, PIN_CLK, PIN_FSYNC4); // Arbitrary SPI pins

#define SILENT_FREQ 1000000

#define ROW1 697.0
#define ROW2 770.0
#define ROW3 852.0
#define ROW4 941.0
#define COL1 1209.0
#define COL2 1336.0
#define COL3 1477.0
#define COL4 1633.0

int rows[4];
int cols[4];

int mapr[16];
int mapc[16];

void setup(void)
{
  randomizer.randomize();
    
  AD1.begin();
  AD1.setFrequency(0, SILENT_FREQ);
  AD2.begin();
  AD2.setFrequency(0, SILENT_FREQ);
  AD3.begin();
  AD3.setFrequency(0, SILENT_FREQ);
  AD4.begin();
  AD4.setFrequency(0, SILENT_FREQ);

  AD1.setMode(MD_AD9833::MODE_SINE);
  AD2.setMode(MD_AD9833::MODE_SINE);
  AD3.setMode(MD_AD9833::MODE_SINE);
  AD4.setMode(MD_AD9833::MODE_SINE);

  rows[0] = ROW1;
  rows[1] = ROW2;
  rows[2] = ROW3;
  rows[3] = ROW4;

  cols[0] = COL1;
  cols[1] = COL2;
  cols[2] = COL3;
  cols[3] = COL4;

  mapr[ 0] = 3;
  mapr[ 1] = 0;
  mapr[ 2] = 0;
  mapr[ 3] = 0;
  mapr[ 4] = 1;
  mapr[ 5] = 1;
  mapr[ 6] = 1;
  mapr[ 7] = 2;
  mapr[ 8] = 2;
  mapr[ 9] = 2;
  mapr[10] = 3;
  mapr[11] = 3;
  mapr[12] = 0;
  mapr[13] = 1;
  mapr[14] = 2;
  mapr[15] = 3;

  mapc[ 0] = 1;
  mapc[ 1] = 0;
  mapc[ 2] = 1;
  mapc[ 3] = 2;
  mapc[ 4] = 0;
  mapc[ 5] = 1;
  mapc[ 6] = 2;
  mapc[ 7] = 0;
  mapc[ 8] = 1;
  mapc[ 9] = 2;
  mapc[10] = 0;
  mapc[11] = 2;
  mapc[12] = 3;
  mapc[13] = 3;
  mapc[14] = 3;
  mapc[15] = 3;
}

void dial_digit(int digit, int on_del=200, int off_del=200){
  AD1.setFrequency(0, rows[mapr[digit]]);
  AD2.setFrequency(0, cols[mapc[digit]]);
  delay(on_del);
   
  AD1.setFrequency(0, SILENT_FREQ);
  AD2.setFrequency(0, SILENT_FREQ);
  delay(off_del);  
}

void dial_random_digits(int count, int on_del=150, int off_del=150, int gap_del=350){
  for(int i = 0; i < count; i++){
    int key = random(0, 10);
    int ran1 = random(0, 50);
    int ran2 = random(0, 50);
    dial_digit(key, on_del + ran1, off_del + ran2);
  }
  int ran3 = random(0, 50);
  delay(gap_del + ran3);
}

void dial_tone(int on_del=2000, int off_del=100){
  AD1.setFrequency(0, 350.0);
  AD2.setFrequency(0, 440.0);
  delay(on_del);

  AD1.setFrequency(0, SILENT_FREQ);
  AD2.setFrequency(0, SILENT_FREQ);
  delay(off_del);
}

void dial_random_phone(int on_del=150, int off_del=150, int gap_del=350){
  dial_digit(1, 250, 500);
  dial_random_digits(3);
  dial_random_digits(3);  
  dial_random_digits(4);  
}

void busy_tone(int times=4){
  for(int i = 0; i < times; i++){
    AD1.setFrequency(0, 480.0);
    AD2.setFrequency(0, 620.0);
    delay(500);

    AD1.setFrequency(0, SILENT_FREQ);
    AD2.setFrequency(0, SILENT_FREQ);
    delay(500);
  }
}

void ring_tone(int times=4){
  for(int i = 0; i < times; i++){
    AD1.setFrequency(0, 480.0);
    AD2.setFrequency(0, 440.0);
    delay(2000);

    AD1.setFrequency(0, SILENT_FREQ);
    AD2.setFrequency(0, SILENT_FREQ);

    if(i < times-1)
      delay(4000);
  }
}

void reorder_tone(int times=8){
  for(int i = 0; i < times; i++){
    AD1.setFrequency(0, 480.0);
    AD2.setFrequency(0, 620.0);
    delay(250);

    AD1.setFrequency(0, SILENT_FREQ);
    AD2.setFrequency(0, SILENT_FREQ);
    delay(250);
  }
}

void error_tone(){
  AD1.setFrequency(0, 913.8);
  delay(380);

  AD1.setFrequency(0, 1428.5);
  delay(276);

  AD1.setFrequency(0, 1776.7);
  delay(380);
  
  AD1.setFrequency(0, SILENT_FREQ);
}

void offhook_tone(int times=20){
  for(int i = 0; i < times; i++){
    AD1.setFrequency(0, 1400.0);
    AD2.setFrequency(0, 2060.0);
    AD3.setFrequency(0, 2450.0);
    AD4.setFrequency(0, 2600.0);
    delay(100);

    AD1.setFrequency(0, SILENT_FREQ);
    AD2.setFrequency(0, SILENT_FREQ);
    AD3.setFrequency(0, SILENT_FREQ);
    AD4.setFrequency(0, SILENT_FREQ);

    delay(100);
  }
}

void loop(void){
  dial_tone();
  dial_random_phone();
  delay(500);
 
  int outcome = random(0, 4);
  switch(outcome){
    case 0:
      busy_tone();      
      break;
    case 1:
      ring_tone();
      break;
    case 2:
      reorder_tone();
      break;
    case 3:
      error_tone();
      delay(4000);
      offhook_tone();
      break;
  }

  delay(2000);

}
