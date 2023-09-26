/*
#########################################################################################
#                                                                                       #
#  config.h for CBM multi-ROM THT version (24-pin & 28-pin ROM)        				        	#
#  To be used with the switchless cbm-multi-ROM THT version                             #
#                                                                                       #
#  Version 1.0                                                                          #
#  https://github.com/xboxpro1                                                          #
#########################################################################################
*/

#pragma once

// include librarys
#include <EEPROM.h>
#include <FastGPIO.h>               // pololu FastGPIO library. see https://github.com/pololu/fastgpio-arduino

// define CBM ids
#define VIC20 1000
#define C64 1001
#define C64C 1002
#define C128 1003
#define C128DCR 1004

// define atmega pins
#define ledPin 8                    // LED pin
#define pin8 8                      // pin 8 (free to use if LED is disabled)
#define pin9 9                      // pin 9 (free to use)
#define pin10 10                    // pin 10 (free to use)
#define resetPin 11                 // reset pin
#define restorePin 12               // restore pin
#define rwPin 13                    // r!w pin
