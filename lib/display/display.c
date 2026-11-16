#include "display.h"
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>


/* Segment byte maps for numbers 0 to 9 and EVERYTHING OFF */
const uint8_t SEGMENT_NUMBERS_MAP[] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99,
                               0x92, 0x82, 0xF8, 0X80, 0X90, 0xFF};
/* Byte maps to select digit 1 to 4 */
const uint8_t SEGMENT_SELECT[] = {0xF1, 0xF2, 0xF4, 0xF8};

// y, E, A, H
const uint8_t SEGMENT_CHARS_MAP[] = {0x91, 0x86, 0x88, 0x89};

void initDisplay() {
  sbi(DDRD, LATCH_DIO);
  sbi(DDRD, CLK_DIO);
  sbi(DDRB, DATA_DIO);
}

// loop through seven segments of LED display and shift the correct bits in the
// data register
void shift(uint8_t val, uint8_t bitorder) {
  uint8_t bit;

  for (uint8_t i = 0; i < NUMBER_OF_SEGMENTS; i++) {
    if (bitorder == LSBFIRST) {
      bit = !!(val & (1 << i));
    } else {
      bit = !!(val & (1 << (7 - i)));
    }
    // write bit to register
    if (bit == HIGH)
      sbi(PORTB, DATA_DIO);
    else
      cbi(PORTB, DATA_DIO);

    // Trigger the clock pin so the display updates
    sbi(PORTD, CLK_DIO);
    cbi(PORTD, CLK_DIO);
  }
}

//Schrijft cijfer naar bepaald segment. Segment 0 is meest linkse.
void writeNumberToSegment(uint8_t segment, uint8_t value) {
  cbi(PORTD, LATCH_DIO);
  shift(SEGMENT_NUMBERS_MAP[value], MSBFIRST);
  shift(SEGMENT_SELECT[segment], MSBFIRST);
  sbi(PORTD, LATCH_DIO);
}


void writeBinaryToSegment(uint8_t segment, uint8_t binary) {
  cbi(PORTD, LATCH_DIO);
  shift(binary, MSBFIRST);
  shift(SEGMENT_SELECT[segment], MSBFIRST);
  sbi(PORTD, LATCH_DIO);
}


void writeNumber(int number) {
  if (number < 0 || number > 9999) return;
  writeNumberToSegment(0, number / 1000);
  writeNumberToSegment(1, (number / 100) % 10);
  writeNumberToSegment(2, (number / 10) % 10);
  writeNumberToSegment(3, number % 10);
  writeBinaryToSegment(3, 0b11111111);
}

void writeNumbers(int number1, int number2, int number3, int number4){
  writeNumberToSegment(0, number1);
  writeNumberToSegment(1, number2);
  writeNumberToSegment(2, number3);
  writeNumberToSegment(3, number4);
  writeBinaryToSegment(3, 0b11111111);
}

void shiftCharacterToSegment(uint8_t segment){
  cbi(PORTD, LATCH_DIO);
  shift(SEGMENT_CHARS_MAP[segment], MSBFIRST);
  shift(SEGMENT_SELECT[segment], MSBFIRST);
  sbi(PORTD, LATCH_DIO);
}

void showEnd(){
  for (size_t i = 0; i < 4; i++)
  {
    shiftCharacterToSegment(i);
  }
  writeBinaryToSegment(3, 0b11111111);
}