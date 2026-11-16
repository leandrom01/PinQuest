#include <avr/io.h>
#include <usart.h>

//DDRB => poorten van de microcontroller definiëren / configureren
//PORTB => poorten van de microcontroller effectief aansturen 

void enableLed(int led) {
    // 0 => eerste led, 1 => tweede led, 2 => derde led, 3 => vierde led
    // 0b00[xxxx]00
    DDRB = DDRB | (1 << (led + 2));
}

void enableLeds(uint8_t led){
    DDRB = DDRB | led; 
}

void enableAllLeds() {
    PORTB  = PORTB | 0b00111100; 
    DDRB = DDRB | 0b00111100;
}

void lightUpLed(int led) {
    PORTB &= ~(1 << (led + 2));  // Set bit LOW → LED ON (because inverted)
}

void lightDownLed(int led) {
    PORTB |= (1 << (led + 2)); 
}

void lightDownAllLeds(){
    PORTB = 0b00111100;
}




