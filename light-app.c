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

#define CAPTURE_FREQUENCY 20 // /s

enum Phase { CALIBRATE, SYNCHRONIZE, INIT, READ };

PROCESS(light_app_process, "light app process");
AUTOSTART_PROCESSES(&light_app_process);

// CALIBRATE
int window[50];
int recorded = 0, threshold = -1;

// SYNCHRONIZE
int periodLengths[5];
int periodsMeasured = 0;
int lastSyncValue = -1;
clock_time_t syncStartTime, periodLength;

// INIT
unsigned char INIT_PATTERN = 'k'; // 01101011
unsigned char last8bits = '\0';

// READ
unsigned char curChar = '\0';
int bitsRead = 0;

enum Phase phase = CALIBRATE;

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
  phase = SYNCHRONIZE;

  PRINTF("Calibration finished, threshold=%d\n", threshold);
}

int getBinaryValue(int intValue) {
  if (threshold == -1) {
    PRINTF("Cannot get binary value, not calibrated yet\n");
    return -1;
  }

  if (intValue < threshold) return 0;
  return 1;
}

void synchronize(int value) {
  int i;
  int syncValue = getBinaryValue(value);

  if (lastSyncValue != -1 && syncValue != lastSyncValue) {
    if (syncStartTime) {
      periodLength = clock_time() - syncStartTime;

      if (periodsMeasured == 5) {
        periodLength = 0;
        for (i = 0; i < 5; i++) {
          periodLength += periodLengths[i];
        }
        periodLength /= 5;

        PRINTF("Synchronization finished, periodLength=%d clock ticks, %lums\n", (int) periodLength,
          ((long) periodLength) * 1000 / CLOCK_SECOND);
        phase = INIT;
      } else {
        periodLengths[periodsMeasured] = periodLength;
        PRINTF("Measured periodLength: %lums\n", ((long) periodLength) * 1000 / CLOCK_SECOND);
        syncStartTime = clock_time();
        periodsMeasured += 1;
      }
    }
    syncStartTime = clock_time();
  }

  lastSyncValue = syncValue;
}

void init(int value) {
  int initValue = getBinaryValue(value);

  last8bits = last8bits << 1;
  last8bits += initValue;

  PRINTF("received %d - last8bits, %u\n", initValue, last8bits);

  if (last8bits == INIT_PATTERN) {
    PRINTF("Initialization finished\n");
    phase = READ;
  }
}

void read(int value) {
  int bitValue = getBinaryValue(value);

  curChar = curChar << 1;
  curChar += bitValue;
  bitsRead += 1;

  if (bitsRead == 8) {
    PRINTF("%c", curChar);
    bitsRead = 0;
    curChar = '\0';
  }
}

char* binaryStringToASCII(const char* binaryString) {
  int i, j;
  int binaryLength, asciiLength, bitsPerChar, offset, charNum, digit;

  bitsPerChar = sizeof(char) * 8;

  binaryLength = strlen(binaryString);
  // check whether the binary string has reasonable amount of bits
  if (binaryLength % bitsPerChar != 0) {
    PRINTF("Binary string does not have a valid length\n");
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

    int value = light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR);

    int waitTime = 0;
    if (phase == CALIBRATE) {
      calibrate(value);
      waitTime = CLOCK_SECOND / CAPTURE_FREQUENCY;
    } else if (phase == SYNCHRONIZE) {
      synchronize(value);
      if (phase == INIT) {
        waitTime = periodLength / 2;
      } else {
        waitTime = 1;
      }
    } else if (phase == INIT) {
      init(value);
      waitTime = periodLength;
    } else if (phase == READ) {
      read(value);
      waitTime = periodLength;
    }

    etimer_set(&et, waitTime);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));


    // PRINTF("light %d %d\n", light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC),
        // light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR));
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/