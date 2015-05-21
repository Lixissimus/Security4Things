/*
 * Copyright (c) 2015, Hasso-Plattner-Institut.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "light-app.h"
#include "sys/etimer.h"
#include "dev/light-sensor.h"
#include "helper.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else /* DEBUG */
#define PRINTF(...)
#endif /* DEBUG */

#define CAPTURE_FREQUENCY 5 // /s

PROCESS(light_app_process, "light app process");
AUTOSTART_PROCESSES(&light_app_process);

int window[50];
int recorded = 0, threshold = -1;
int calibrated = 0;

void calibrate(int newValue) {
  int min = 10000, max = -1;
  int i;

  if (recorded < 50) {
    PRINTF("Calibration value: %d\n", newValue);
    window[recorded] = newValue;
    recorded++;
    return;
  }
  
  for (i = 0; i < sizeof(window) / sizeof(int); i++) {
    if (window[i] < min) min = window[i];
    if (window[i] > max) max = window[i];
  }

  threshold = (min + max) / 2;
  calibrated = 1;

  PRINTF("Calibration finished, threshold=%d\n", threshold);
}

int getBinaryValue(int intValue) {
  if (calibrated == 0) {
    PRINTF("Cannot get binary value, not calibrated yet\n");
    return -1;
  }

  if (intValue < threshold) return 0;
  return 1;
}

void onNewLightValue(int value) {
  int binaryValue;

  if (calibrated == 0) {
    calibrate(value);
    return;
  }

  binaryValue = getBinaryValue(value);
  PRINTF("Received %d (%d)\n", binaryValue, value);
}

char* binaryStringToASCII(const char* binaryString) {
  int i, j;
  int binaryLength, asciiLength, bitsPerChar, offset, charNum, digit;

  bitsPerChar = sizeof(char) * 8;

  binaryLength = strlen(binaryString);
  // check whether the binary string has reasonable amount of bits
  if (binaryLength % bitsPerChar != 0) {
    PRINTF("Binary string does not have a valid length");
    return NULL;
  }

  asciiLength = binaryLength / bitsPerChar;

  // allocate memory for the result: #asciiLength chars + null-byte
  char* asciiString = (char*)malloc(sizeof(char) * (asciiLength + 1));

  for (i = 0; i < asciiLength; i++) {
    charNum = 0;
    offset = i * bitsPerChar;
    // loop over bits that form a char
    for (j = 0; j < bitsPerChar; j++) {
      // this converts char '0' or '1' to an int (0 or 1)
      digit = binaryString[j + offset] - '0';
      // shift the bit to its appropriate position and add to previous result
      charNum += digit << (bitsPerChar - j - 1);
    };

    asciiString[i] = charNum;
  }
  // finish the string
  asciiString[asciiLength] = '\0';

  return asciiString;
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(light_app_process, ev, data)
{
  static struct etimer et;
  static int i;

  PROCESS_BEGIN();

  SENSORS_ACTIVATE(light_sensor);

  PRINTF("Countdown for calibration...");
  i = 3;
  for (i = 3; i >= 0; i--) {
    etimer_set(&et, CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    PRINTF(" %d...", i);
  }
  PRINTF("go!\n");

  while(1) {
    etimer_set(&et, CLOCK_SECOND / CAPTURE_FREQUENCY); 
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    int value = light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR);
    onNewLightValue(value);
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/