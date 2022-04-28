#include <Arduino.h>

byte aCols[8] = {A5, 2, 3, 4, 5, 6, 7, 8}; // note col A5 is the extra one linked to DP
byte aRows[3] = {9, 10, 11};

byte ledSelect[8] = {12, 13, A0, A1, A2, A3, A7, A4};  // note that A6 and A7 are not used at present. Can delete them.
byte ledSelect7[8] = {12, 13, A0, A1, A4, A2, A3, A7}; // note that A6 and A7 are not used at present. Can delete them.
