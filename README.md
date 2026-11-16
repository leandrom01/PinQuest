# PinQuest

A PIN-guessing game for the Arduino Uno, written in C directly on the AVR registers (no Arduino helper functions). Made for a programming course at KdG, spring 2025.

The board picks a random 4-digit PIN; you crack it with the buttons, one digit at a time.

## Hardware

- Arduino Uno (ATmega328P)
- Velleman VMA209 multi-function shield: 4-digit seven-segment display, 3 buttons, 4 LEDs, buzzer, potentiometer

## How to play

1. Open a serial monitor (9600 baud).
2. Turn the potentiometer to create a random seed, then press button 1. The LEDs count down to the start.
3. Button 1 raises the selected digit (it blinks), button 2 moves to the next digit, button 3 checks the PIN.
4. After every try the serial monitor shows each digit as correct, or wrong with a hint (higher or lower). The buzzer plays a happy or a sad melody, and the display shows `yEAH` when you crack it.

## How it works

- `lib/display`: my display library. It drives the four digits through two shift registers by sending the segment and digit bits one by one.
- `src/main.c`: the game. Buttons use a pin-change interrupt, Timer0 redraws the display and makes the selected digit blink, Timer2 generates the buzzer notes, and the ADC reads the potentiometer for the seed.
- `lib/VMA209`, `lib/WPSH209`: small helpers for the shield's LEDs and buzzer.
- `lib/usart`: serial-port helpers provided by the course. The buzzer timing in `main.c` follows the method taught in class.

## Build

With [PlatformIO](https://platformio.org/):

```sh
pio run -t upload
pio device monitor -b 9600
```
