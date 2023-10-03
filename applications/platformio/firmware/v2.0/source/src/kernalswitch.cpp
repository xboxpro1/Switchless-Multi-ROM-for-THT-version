/*
#########################################################################################
#                                                                                       #
#  CBM kernal switcher sketch for multi-ROM THT version (24-pin & 28-pin ROM)           #
#  To be used with the switchless cbm-multi-ROM THT version                             #
#                                                                                       #
#  Version 1.4                                                                          #
#  https://github.com/retronynjah                                                       #
#  modified by xboxpro1                                                                 #
#  Version 2.0                                                                          #
#  https://github.com/xboxpro1                                                          #
#########################################################################################
*/

#include "config.h"                                                                         // include definitions and librarys

//  set commodore cbm id, possible ids
//  VIC20 - (24pin ROM, 8k ROM, flashbank -> 0) 
//  C64 - (longboard, 24pin ROM, 8k ROM, flashbank -> 0)
//  C64C - (shortboard, 28pin ROM, 16k ROM, flashbank -> 1)
//  C128 - (28pin ROM, 16k ROM, flashbank -> 1)
//  C128DCR - (28pin ROM, 32k ROM, flashbank -> 2)

#define CBMID C64C                                                                          // set CBM ID

// enable LED for blinking
#define LED                                                                                 // enable LED, commend out to disable

// nothing to setup below
#ifndef CBMID
  #define CBMID C64                                                                         // for security reasons in case something goes wrong with defining CBMID
#endif

// searchString is the command that the kernalselector minikernal sends to the THT ROM switcher.
// The command will be followed by a single byte ROM number between 1 (0x01) and 10 (0x0a)
// flashbank selects 8k, 16k, or 32k ROM

// VIC20 config
#if CBMID == VIC20
  byte searchString[] = {0x43,0x42,0x4D,0x52,0x4F,0x4D,0x32,0x30,0x23};                     // searchstring is "CBMROM20#"
  #define flashBank 0                                                                       // 8k ROM
  #define maxROM  10                                                                        // max minikernal ROM entries 
#endif

// C64 longboard config
#if CBMID == C64
  byte searchString[] = {0x43,0x42,0x4D,0x52,0x4F,0x4D,0x36,0x34,0x23};                     // searchstring is "CBMROM64#"
  #define flashBank 0                                                                       // 8k ROM
  #define maxROM  10                                                                        // max minikernal ROM entries
#endif

// C64C shortboard config
#if CBMID == C64C
  byte searchString[] = {0x43,0x42,0x4D,0x52,0x4F,0x4D,0x36,0x34,0x23};                     // searchstring is "CBMROM64#"
  #define flashBank 1                                                                       // 16k ROM
  #define maxROM  10                                                                        // max minikernal ROM entries
#endif

// C128 config
#if CBMID == C128
  byte searchString[] = {0x43,0x42,0x4D,0x52,0x4F,0x4D,0x31,0x32,0x38,0x23};                // searchstring is "CBMROM128#"
  #define flashBank 1                                                                       // 16k ROM
  #define maxROM  10                                                                        // max minikernal ROM entries
#endif

// C128DCR config
#if CBMID == C128DCR
  byte searchString[] = {0x43,0x42,0x4D,0x52,0x4F,0x4D,0x31,0x32,0x38,0x44,0x43,0x52,0x23}; // searchstring is "CBMROM128DCR#"
  #define flashBank 2                                                                       // 32k ROM
  #define maxROM  10                                                                        // max minikernal ROM entries
#endif

// variables
byte commandLength = sizeof(searchString);
byte savedROM;
byte bytesCorrect = 0;
byte dataByte = 0x00;
long restorepressed;
volatile bool rwstate;
bool restorestate;
bool resetstate;
bool restoreholding = false;
bool inmenu = false;

#ifdef LED
  byte blinkCounter = 0;
  unsigned long ledtiming = 0;
#endif

// interrupt vector handling
ISR(PCINT0_vect){                                             // handle pin change interrupt for Port B0 - B5, Pin 8 to 13
  rwstate = FastGPIO::Pin<rwPin>::isInputHigh();              // read state of rw pin (D13); indicate that data on port d is available
}

// functions
void pciSetup(byte pin) {                                     // enable Pin Change Interrupt for requested pin
  *digitalPinToPCMSK(pin) |= bit(digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR |= bit(digitalPinToPCICRbit(pin));                    // clear any outstanding interrupt
  PCICR |= bit(digitalPinToPCICRbit(pin));                    // enable interrupt for the group
}

void switchrom(int romnumber, bool doreset){                  // ROM switch function, romnumber & do reset
  
   if (doreset){                                              // if reset = true
      FastGPIO::Pin<resetPin>::setOutputLow();                // hold reset active-low
      delay(100);
   }
    
   #if flashBank == 0
     PORTC = romnumber;                                       // set address lines A13-A18 for selected Bank 0 8k ROM (Port C0 - C5)
   #else
     PORTC = romnumber << flashBank;                          // bitshift romnumber to set address lines A14-A18 for selected Bank 1 16k ROM or address lines A15-A18 for selected Bank 2 32k ROM (Port C0 - C5)
   #endif

  if ((romnumber != savedROM) && (romnumber != 0)){           // if switching to a new kernal that isn't the menu kernal - save kernal to EEPROM address 0
    EEPROM.write(0, romnumber);
    savedROM = romnumber;
  }

    if (romnumber == 0){                                      // if romnumber = 0 we are in minikernal menu mode, listen for command string
      inmenu = true;
    }
    else {
      inmenu = false;                                         // if romnumber =! 0 we are in operational mode, listen for restore key
      #ifdef LED
        blinkCounter = romnumber << 1;                        // set blink counter to toggle the LED, if romnumber is 2, we have to toggle 4 times, 2 LOW, 2 HIGH, bitshift left 1 is like multiply by 2, bitshift is faster
      #endif
    }
    
    if (doreset) {
      delay(200);
      FastGPIO::Pin<resetPin>::setInput();                    // release reset
      delay(500);                                             // give system some time to reset before entering main loop again
    }
}

#ifdef LED                                                 
  void ledcontrol(){                                                  // LED control function, indicate ROM number and if restore key is pressed, restore key interrupts blinking
    if (blinkCounter > 0 && restoreholding == false) {                // if kernal has just been switched we can do some led blinking now to indicate selected kernal                                        
          if (ledtiming == 0) {                                       // first turn LED off and start counting
            FastGPIO::Pin<ledPin>::setOutputValueLow();               // LED off
            ledtiming = millis() + 250;                               // blink timing, toggle after 250 ms
            blinkCounter--;                                           // decrease blink counter to ensure that the last toggle is LED on
           } 
           else {
            if (millis() > ledtiming) {                               // time to toggle
                FastGPIO::Pin<ledPin>::setOutputValueToggle();        // toggle LED
                ledtiming = millis() + 250;                           // blink timing, toggle every 250 ms
                blinkCounter--;                                       // decrease blink counter
                } 
             }
     }
     else {                                                           // blink counter is 0, no blinking any more, reset blink timing, indicate if restore key is pressed
        ledtiming = 0;                                                // reset LED blink timing
        if (restoreholding) {                                         // turn LED off when restore is pressed
              FastGPIO::Pin<ledPin>::setOutputValueLow();             // LED off
        }
        else {                                                        // turn LED on when restore is released
              FastGPIO::Pin<ledPin>::setOutputValueHigh();            // LED on
        }
     } 
 }
#endif

// setup
void setup() {                                              // setup

  FastGPIO::Pin<resetPin>::setOutputLow();                  // set reset pin (low), keep reset active (low) during setup, connected to reset signal
  
  DDRD = 0x00;                                              // set data pins portD (D0-D7) as inputs (command string data)
  DDRC = 0x3F;                                              // set address pins portC (C0-C5 ~ 0x3F) as outputs (address lines A13 - A18); pin22 -> flashOE (C0-C6 ~ 0x7F) if ext reset is disabled is fuse

  PORTC = 0x00;                                             // set address lines A13 - A18 (C0-C5 ~ 0x00) low; pin22 -> flashOE high (C6 ~ 0x40) if ext reset is disabled in fuse
  
  FastGPIO::Pin<rwPin>::setInput();                         // set rw pin as input, connected to r!w signal
  
  FastGPIO::Pin<restorePin>::setInput();                    // set restore pin as input, connected to restore key   
    
  #ifdef LED
    FastGPIO::Pin<ledPin>::setOutputHigh();                 // set LED pin as output and switch LED on
  #else
    FastGPIO::Pin<pin8>::setInputPulledUp();                // according to the atmel data sheet it is recommended to ensure that unused pins have a defined level
  #endif

  FastGPIO::Pin<pin9>::setInputPulledUp();                  // according to the atmel data sheet it is recommended to ensure that unused pins have a defined level
  FastGPIO::Pin<pin10>::setInputPulledUp();                 // according to the atmel data sheet it is recommended to ensure that unused pins have a defined level

  #ifdef PBVARIANT
    FastGPIO::Pin<pin23>::setInputPulledUp();               // according to the atmel data sheet it is recommended to ensure that unused pins have a defined level
    FastGPIO::Pin<pin24>::setInputPulledUp();               // according to the atmel data sheet it is recommended to ensure that unused pins have a defined level
    FastGPIO::Pin<pin25>::setInputPulledUp();               // according to the atmel data sheet it is recommended to ensure that unused pins have a defined level
    FastGPIO::Pin<pin26>::setInputPulledUp();               // according to the atmel data sheet it is recommended to ensure that unused pins have a defined level
  #endif

  savedROM = EEPROM.read(0);                                // retrieve last used ROM from ATMEGA EEPROM and switch ROM using address lines A13-A18
  if (savedROM > maxROM){                                   // if saved ROM > maxROM, reset to minikernal menu ROM 0
    savedROM = 0;
  }
  
  switchrom(savedROM, false);                               // switch to saved ROM using address lines A13-A18 without reset, we already hold reset low

  FastGPIO::Pin<resetPin>::setInput();                      // release reset
  delay(500);                                               // give system some time to finish reset before entering loop

  pciSetup(rwPin);                                          // enable pin change interrupt on rw pin connected to r!w signal
  
}                                                           // setup end

// main loop
void loop() {                                               // main loop
  
  if (inmenu) {                                             // while in menu mode, listen for command string
    if (rwstate == HIGH) {                                  // interrupt occurred, data on port d is available
      dataByte = PIND;                                      // get data byte from port d
      rwstate = LOW;                                        // reset rw interrupt state
      if (bytesCorrect == commandLength) {                  // we have already found our command string, this byte must be the ROM number
        if ((dataByte >= 1) && (dataByte <= maxROM)) {      // valid numbers are 1 - maxROM
          switchrom(dataByte, true);                        // ROM number within valid range, switch ROM using address lines A13-A18 and do a reset
        } else {
          bytesCorrect = 0;
        }
      }
      else if (dataByte == searchString[bytesCorrect]) {    // we don't have full command string yet, check if current byte is what we are looking for
        bytesCorrect++;                                     // if current byte is correct, increase bytesCorrect to check for next character
      } else {
        bytesCorrect = 0;
      }
    }

    resetstate = FastGPIO::Pin<resetPin>::isInputHigh();    // check if reset pin is active-low
    if (resetstate == LOW) {                                // reset is active, other switch might be active and doing a reset or user is resetting
      switchrom(savedROM, false);                           // switch ROM to give up menu in case we're in menu and don't perform another reset as a reset is already in progress
    }
   
  } 
      else {                                                           // while in operational mode, listen for restore key
        restorestate = FastGPIO::Pin<restorePin>::isInputHigh();       // check if restore key is pressed
        if (restorestate == LOW) {                                     // restore key pressed
            if (restoreholding == false) {                             // start counting restore key hold time
            restoreholding = true;
            restorepressed = millis();
            }
            if ((millis() - restorepressed) > 2000) {                  // if restore key hold time is more then 2 seconds switch to minikernal menu ROM and do a reset
             restoreholding = false;
             #ifdef LED
               blinkCounter = 0;                                       // end LED blinking if in progress
             #endif
             switchrom(0, true);                                       // switch ROM to minikernal menu ROM and do a reset
           }
        } 
        else {                                                         // restore key not held
          if (restoreholding){                                         // restore key has just been released
            restoreholding = false;
          }
      }
      #ifdef LED
        ledcontrol();                                                  // LED control function, indicate ROM number and if restore key is pressed
      #endif 
    }
 }                                                                     // main loop end