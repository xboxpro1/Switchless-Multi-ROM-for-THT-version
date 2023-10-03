/*
#########################################################################################
#                                                                                       #
#  config.h for CBM multi-ROM THT version (24-pin & 28-pin ROM)                         #
#  To be used with the switchless cbm-multi-ROM THT version                             #
#                                                                                       #
#  Version 1.0                                                                          #
#  https://github.com/xboxpro1                                                          #
#########################################################################################
*/

#pragma once

// include librarys
#include <Arduino.h>
#include <EEPROM.h>
#include <FastGPIO.h>               // pololu FastGPIO library. see https://github.com/pololu/fastgpio-arduino

// define mcu variant for additional pins
#if defined(__AVR_ATmega48PB__) || defined(__AVR_ATmega88PB__) || defined(__AVR_ATmega168PB__) || defined(__AVR_ATmega328PB__)
    #define PBVARIANT
#endif

// define CBM ids
#define VIC20 1000
#define C64 1001
#define C64C 1002
#define C128 1003
#define C128DCR 1004

// define fastgpio atmega pins
#define ledPin IO_B0                // LED pin
#define pin8 IO_B0                  // pin 8 (free to use if LED is disabled)
#define pin9 IO_B1                  // pin 9 (free to use)
#define pin10 IO_B2                 // pin 10 (free to use)
#define resetPin IO_B3              // reset pin 11
#define restorePin IO_B4            // restore pin 12
#define rwPin IO_B5                 // rw pin 13
#ifdef PBVARIANT
    #define pin23 IO_E0             // pin 23 pb-variant
    #define pin24 IO_E1             // pin 24 pb-variant
    #define pin25 IO_E2             // pin 25 pb-variant
    #define pin26 IO_E3             // pin 26 pb-variant
#endif
