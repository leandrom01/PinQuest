#include <util/delay.h> //deze library bevat onder andere de functie _delay_ms(..) om de uitvoering van je programma even te pauzeren
#include <avr/io.h>
#include <usart.h>

#define NUMBER_OF_LEDS 4

//Deze functie geeft een byte terug waarbij er ééntjes staan op de plek waar er iets moet veranderen
uint8_t makeMaskBasedOnLedNumber(int lednumber)
{
  if (lednumber <= 0 || lednumber > NUMBER_OF_LEDS)
    return 0;
  return (0b1000000 >> lednumber);
}

uint8_t makeMask(uint8_t zeroPositionedMaskLsbIsD1)
{
  uint8_t reversedBits = 0;

  for (uint8_t teller = 0; teller < 4; teller++)
  {
    reversedBits = reversedBits << 1;
    reversedBits = reversedBits + zeroPositionedMaskLsbIsD1 % 2;
    zeroPositionedMaskLsbIsD1 = zeroPositionedMaskLsbIsD1 >> 1;
  }
  return (reversedBits << 2);
}

//Ik gebruik de nummertjes die op het expansion shield staan => D1-D4
void enableLed(int lednumber)
{
  DDRB |= makeMaskBasedOnLedNumber(lednumber);
}

void lightUpLed(int lednumber)
{ //opgelet: ge-enabelde leds staan ook ineens aan (want 0 = aan)
  PORTB &= ~makeMaskBasedOnLedNumber(lednumber);
}

//Ik gebruik de nummertjes die op het expansion shield staan => D1-D4
void lightDownLed(int lednumber)
{
  PORTB |= makeMaskBasedOnLedNumber(lednumber); //Zorg ook dat je deze code goed begrijpt!
}

//Ik gebruik de nummertjes die op het expansion shield staan => D1-D4
//0b1010 is D2 en D4
void enableLeds(uint8_t zeroPositionedMaskLsbIsD1)
{
  DDRB |= makeMask(zeroPositionedMaskLsbIsD1);
}

//Ik gebruik de nummertjes die op het expansion shield staan => D1-D4
//0b1010 is D2 en D4
void lightUpLeds(uint8_t zeroPositionedMaskLsbIsD1)
{
  PORTB &= ~makeMask(zeroPositionedMaskLsbIsD1);
}

//Ik gebruik de nummertjes die op het expansion shield staan => D1-D4
//0b1010 is D2 en D4
void lightDownLeds(uint8_t zeroPositionedMaskLsbIsD1)
{
  PORTB |= makeMask(zeroPositionedMaskLsbIsD1);
}

void enableAllLeds()
{
  enableLeds(0b1111);
}

void lightUpAllLeds()
{
  lightUpLeds(0b1111);
}

void lightDownAllLeds()
{
  lightDownLeds(0b1111);
}

void toggleLed(int lednumber)
{ //opgelet: ge-enabelde leds staan ook ineens aan (want 0 = aan)
  PORTB ^= makeMaskBasedOnLedNumber(lednumber);
}