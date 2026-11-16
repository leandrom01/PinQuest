#include <avr/interrupt.h>
#include <avr/io.h>
#include <display.h>
#include <usart.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <VMA209.h>
#include <stdbool.h>

// Music note frequencies
#define C4  261.630
#define D4  293.660
#define E4  329.630
#define F4  349.230
#define G4  392.000
#define A4  440.000
#define B4  493.880
#define C5  523.250
#define D5  587.330
#define E5  659.250
#define F5  698.460
#define G5  783.990

// ===== STRUCT IMPLEMENTATIE =====
typedef struct {
    int numbersToGuess[4];
    bool gameInProgress;
    bool endGame;
    volatile bool seedGenerated;
    volatile uint16_t seed;
    volatile int numberToGuess;
} GameInfo;

typedef struct {
    int numbers[4];
    int selectedNumber;
    int amountOfTries;
} PlayerInfo;

typedef struct {
    float frequencies[10];
    int currentNote;
} MelodyState;

GameInfo* gameInfo = NULL;        // Pointer naar game informatie
PlayerInfo* playerInfo = NULL;    // Pointer naar speler informatie  
MelodyState* melodyState = NULL;  // Pointer naar melody state

// Buzzer variables (following professor's approach)
int totalTimerCyclesToExecute = 0;
int cyclesBeforeBuzzerToggle = 0;
int buzzerToggleWithinThisTimerCycles = 0;

// Melody variables
volatile uint8_t currentMelodyNote = 0;
volatile uint8_t melodyLength = 0;
volatile bool playingMelody = false;
volatile bool noteFinished = true;

volatile uint8_t blinkCounter = 0;
volatile bool showDigit = true;

// Melody definitions
float wrongMelody[] = {E4, D4, C4}; // Descending sad notes
float correctMelody[] = {C5, E5, G5, C5}; // Happy ascending notes

// ===== FUNCTIES MET BY VALUE EN BY REFERENCE =====

// BY VALUE: functie krijgt kopie van de waarde
void printAttemptResult(int attemptNumber, bool isCorrect) {
    if (isCorrect) {
        printf("\nPoging %d => GEFELICITEERD! U heeft het geraden. :)", attemptNumber);
    } else {
        printf("\nPoging %d => Jammer genoeg was het verkeerd.", attemptNumber);
    }
}

// BY REFERENCE: functie krijgt pointer naar originele variabele
void updateSelectedNumber(int* selectedNum) {
    (*selectedNum)++;
    if (*selectedNum >= 4) {
        *selectedNum = 0;
    }
}

// BY REFERENCE: functie werkt met pointer naar PlayerInfo struct
void resetPlayerInfo(PlayerInfo* player) {
    for (int i = 0; i < 4; i++) {
        player->numbers[i] = 0;
    }
    player->selectedNumber = 0;
    player->amountOfTries = 0;
}

// BY REFERENCE: functie werkt met pointer naar GameInfo struct
void resetGameInfo(GameInfo* game) {
    for (int i = 0; i < 4; i++) {
        game->numbersToGuess[i] = 0;
    }
    game->gameInProgress = false;
    game->endGame = false;
    game->seedGenerated = false;
    game->seed = 0;
    game->numberToGuess = 0;
}

// BY REFERENCE: functie werkt met pointer naar MelodyState struct
void initMelodyState(MelodyState* melody) {
    // Initialize with default frequencies (can be customized)
    melody->frequencies[0] = C4;
    melody->frequencies[1] = D4;
    melody->frequencies[2] = E4;
    melody->frequencies[3] = F4;
    melody->frequencies[4] = G4;
    melody->frequencies[5] = A4;
    melody->frequencies[6] = B4;
    melody->frequencies[7] = C5;
    melody->frequencies[8] = D5;
    melody->frequencies[9] = E5;
    melody->currentNote = 0;
}

// ===== POINTER GEBRUIK =====
void showChoicesWithPointer() {
    // Gebruik pointer om door arrays te gaan
    int* numbersPtr = playerInfo->numbers;
    int* guessPtr = gameInfo->numbersToGuess;
    
    printf("\n---------------------------------------------");
    printf("\n|  %-6d  |  %-6d  |  %-6d  |  %-6d  |", 
           *(numbersPtr), *(numbersPtr+1), *(numbersPtr+2), *(numbersPtr+3));
    printf("\n ------------------------------------------ ");

    char result0[10], result1[10], result2[10], result3[10];

    for (int i = 0; i < 4; i++) {
        if (*(numbersPtr + i) == *(guessPtr + i)) {
            snprintf(i == 0 ? result0 
                : i == 1 ? result1 
                : i == 2 ? result2 
                : result3, 
                10, 
                "Juist");
        } else {
            snprintf(i == 0 ? result0 : 
                    i == 1 ? result1 : 
                    i == 2 ? result2 : 
                    result3, 
                     10, 
                     "Fout %s", 
                     *(numbersPtr + i) > *(guessPtr + i) ? "↓" : "↑");
        }
    }

    printf("\n|  %-6s  |  %-6s  |  %-6s  |  %-6s  |", 
           result0, result1, result2, result3);
    printf("\n---------------------------------------------");
}

uint16_t readADCaverage() {
    uint32_t sum = 0;
    for (int i = 0; i < 150; i++) {
        ADCSRA |= (1<<ADSC); 
        loop_until_bit_is_clear(ADCSRA, ADSC); 
        sum += ADC;
    }
    return sum / 150;
}

void startGameCountdownLeds(){
    lightUpAllLeds();
    _delay_ms(500);
    lightDownLed(4);
    _delay_ms(500);
    lightDownLed(3);
    _delay_ms(500);
    lightDownLed(2);
    _delay_ms(500);
    lightDownLed(1);
    _delay_ms(500);
    gameInfo->gameInProgress = true;
}

// Buzzer functions following professor's approach
void enableBuzzer() {
    DDRD |= (1 << PD3); // Set PD3 as output for buzzer
    PORTD |= (1 << PD3); // Start with buzzer OFF (high = off for professor's logic)
}

void makeTimer2GenerateBuzzerTone(float frequency, double activeTimeInSeconds) {
    double timerCycleDurationInSeconds = (1.0 / ((1.0 / 16000000.0) * 8.0 * 255.0));
    cyclesBeforeBuzzerToggle = (int)((timerCycleDurationInSeconds / frequency) / 2.0);
    totalTimerCyclesToExecute = (int)(activeTimeInSeconds * timerCycleDurationInSeconds);
    buzzerToggleWithinThisTimerCycles = 0;
}

void playNextNote() {
    if (playingMelody && currentMelodyNote < melodyLength) {
        float* currentMelody = (melodyLength == 3) ? wrongMelody : correctMelody;
        float frequency = currentMelody[currentMelodyNote];
        
        // Play note for 250ms
        makeTimer2GenerateBuzzerTone(frequency, 0.25);
        noteFinished = false;
        currentMelodyNote++;
    } else {
        // Melody finished
        playingMelody = false;
        currentMelodyNote = 0;
        noteFinished = true;    
        // Make sure buzzer is off
        PORTD |= (1 << PD3);
    }
}

void playMelody(float* melody, uint8_t length) {
    // Always allow starting a new melody (interrupt current one if playing)
    playingMelody = true;
    currentMelodyNote = 0;
    melodyLength = length;
    noteFinished = true;
    
    // Stop current buzzer activity immediately
    totalTimerCyclesToExecute = 0;
    PORTD |= (1 << PD3); // Turn off buzzer
    
    playNextNote();
}

void playWrongMelody() {
    playMelody(wrongMelody, 3);
}

void playCorrectMelody() {
    playMelody(correctMelody, 4);
}

void checkPin(){
    playerInfo->amountOfTries++;
    
    bool isCorrect = (playerInfo->numbers[0] == gameInfo->numbersToGuess[0] &&
                     playerInfo->numbers[1] == gameInfo->numbersToGuess[1] &&
                     playerInfo->numbers[2] == gameInfo->numbersToGuess[2] && 
                     playerInfo->numbers[3] == gameInfo->numbersToGuess[3]);
    
    // BY VALUE call - toon resultaat van deze poging
    printAttemptResult(playerInfo->amountOfTries, isCorrect);
    
    if(isCorrect){
        playCorrectMelody();
        gameInfo->gameInProgress = false;
        gameInfo->endGame = true;
    } else {
        playWrongMelody();
    }
    
    // Toon altijd de tabel na elke poging
    showChoicesWithPointer();
}

// Timer2 interrupt following professor's approach
ISR(TIMER2_OVF_vect) {
    if (totalTimerCyclesToExecute > 0) {
        totalTimerCyclesToExecute--;
        if (buzzerToggleWithinThisTimerCycles == 0) {
            // Toggle buzzer
            PORTD = PORTD ^ (1 << PD3);
            buzzerToggleWithinThisTimerCycles = cyclesBeforeBuzzerToggle;
        } else {
            buzzerToggleWithinThisTimerCycles--;
        }
    } else {
        // Deactivate buzzer
        PORTD |= (1 << PD3);
        
        // Check if we need to play next note in melody
        if (playingMelody && !noteFinished) {
            noteFinished = true;
            // Small delay will be handled in main loop
        }
    }
}

// Button interrupt
ISR(PCINT1_vect){
    if(!gameInfo->seedGenerated){
        if (((PINC & 0b00000010) == 0)){
            _delay_us(1500); // debounce
            if (((PINC & 0b00000010) == 0)){
                gameInfo->seed = (readADCaverage() * 9999UL) / 1023;
                gameInfo->seedGenerated = true;
                srand(gameInfo->seed);
                gameInfo->numberToGuess = rand() % 9999;
                gameInfo->numbersToGuess[0] = (gameInfo->numberToGuess / 1000);
                gameInfo->numbersToGuess[1] = ((gameInfo->numberToGuess / 100) % 10);
                gameInfo->numbersToGuess[2] = ((gameInfo->numberToGuess / 10) % 10);
                gameInfo->numbersToGuess[3] = (gameInfo->numberToGuess % 10);
                startGameCountdownLeds();
            }
        }
    }
    if(gameInfo->gameInProgress){
        if (((PINC & 0b00000010) == 0) || ((PINC & 0b00000100) == 0) || ((PINC & 0b00001000) == 0)){
            _delay_us(1500); // debounce
            if (((PINC & 0b00000010) == 0)){
                playerInfo->numbers[playerInfo->selectedNumber]++;
                if(playerInfo->numbers[playerInfo->selectedNumber] == 10){
                    playerInfo->numbers[playerInfo->selectedNumber] = 0;
                }
            }
            if (((PINC & 0b00000100) == 0)){
                updateSelectedNumber(&(playerInfo->selectedNumber));  // BY REFERENCE call
            }
            if (((PINC & 0b00001000) == 0)){
                checkPin();
            }
        }
    }
}

// Timer0 interrupt for display
ISR(TIMER0_OVF_vect)
{
    if (gameInfo->gameInProgress) {
        blinkCounter++;
        if (blinkCounter >= 90) {
            blinkCounter = 0;
            showDigit = !showDigit;
        }

        int toDisplay[4];
        for (int i = 0; i < 4; i++) {
            if (i == playerInfo->selectedNumber && !showDigit) {
                toDisplay[i] = 10; 
            } else {
                toDisplay[i] = playerInfo->numbers[i];
            }
        }
        writeNumbers(toDisplay[0], toDisplay[1], toDisplay[2], toDisplay[3]);
    }
    if(!gameInfo->gameInProgress && gameInfo->endGame){
        showEnd();
    }
}

void initTimer0()
{
    TCCR0A |= _BV(WGM00) | _BV(WGM01); 
    TCCR0B |=  _BV(CS02); 
    TIMSK0 |= _BV(TOIE0); 
    sei(); 
}

void initTimer2() {
    // Following professor's approach exactly
    // FAST PWM Mode
    TCCR2A |= _BV(WGM20) | _BV(WGM21);
    // Prescaler = 8
    TCCR2B |= _BV(CS21);
    // Enable overflow interrupt
    TIMSK2 |= _BV(TOIE2);
    sei();
}

void initButtons(){
    DDRC = 0b11110001;       
    PORTC |= 0b00001110;     
    PCICR |= 0b00000010;     
    PCMSK1 |= 0b00001110;
}

void initADC(){ 
    ADMUX |= (1<<REFS0); 
    ADCSRA |= (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); 
    ADCSRA |= (1<<ADEN); 
}

int main(void)
{ 
    // ===== DYNAMISCHE GEHEUGENALLOCATIE MET CALLOC =====
    // Alloceer geheugen voor GameInfo struct
    gameInfo = (GameInfo*)calloc(1, sizeof(GameInfo));
    if (gameInfo == NULL) {
        printf("Fout: Geen geheugen beschikbaar voor GameInfo!");
        return -1;
    }
    
    // Alloceer geheugen voor PlayerInfo struct
    playerInfo = (PlayerInfo*)calloc(1, sizeof(PlayerInfo));
    if (playerInfo == NULL) {
        printf("Fout: Geen geheugen beschikbaar voor PlayerInfo!");
        free(gameInfo); // Maak eerder gealloceerd geheugen vrij
        return -1;
    }
    
    // Alloceer geheugen voor MelodyState struct
    melodyState = (MelodyState*)calloc(1, sizeof(MelodyState));
    if (melodyState == NULL) {
        printf("Fout: Geen geheugen beschikbaar voor MelodyState!");
        free(gameInfo);   // Maak eerder gealloceerd geheugen vrij
        free(playerInfo);
        return -1;
    }
    
    // Initialiseer de structs
    resetGameInfo(gameInfo);        // BY REFERENCE call
    resetPlayerInfo(playerInfo);    // BY REFERENCE call
    initMelodyState(melodyState);   // BY REFERENCE call
    
    initUSART(); 
    initDisplay();
    initTimer0(); 
    initTimer2();  // Initialize Timer2 for buzzer
    initButtons();
    initADC();
    enableBuzzer(); // Initialize buzzer pin
    enableAllLeds();
    lightDownAllLeds();

    printf("Draai aan de potentiometer een seed te creeeren en daarna druk op knop1 om te beginnen.");

    while(1){
        if (playingMelody && noteFinished && currentMelodyNote < melodyLength) {
            _delay_ms(100); 
            playNextNote();
        }
        
        // Als het spel eindigt, maak geheugen vrij
        if (gameInfo->endGame) {
            _delay_ms(10000);
            
            // FREE DYNAMISCH GEHEUGEN - alle 3 structs
            free(gameInfo);
            free(playerInfo);
            free(melodyState);
            
            // Set pointers to NULL for safety
            gameInfo = NULL;
            playerInfo = NULL;
            melodyState = NULL;
            break;
        }
    }
    
    return 0;
}