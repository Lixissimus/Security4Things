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
#include "crc32.h"
#include "sys/etimer.h"
#include "dev/light-sensor.h"
#include "k-means.h"
#include "hamming.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else /* DEBUG */
#define PRINTF(...)
#endif /* DEBUG */

#define CAPTURE_FREQUENCY 20 // /s
// number of clusters to classify light values
#define K_CLUSTERS 2
// number of values to build kmeans clusters
#define KMEANS_VALUES 50

#define LEDS_GREEN    1
#define LEDS_BLUE     2
#define LEDS_RED      4
#define LEDS_ALL      7

#define READ_X_BYTES(x, id, newPhase, printChars) \
if (initializedBuffer##id == 0) { \
  readBuffer = malloc(sizeof(char) * x); \
  readBufferBytesRead = 0; \
  initializedBuffer##id = 1; \
} \
readBufferBytesRead += read(value, readBuffer, readBufferBytesRead, printChars); \
if (readBufferBytesRead >= x) { \
  phase = newPhase; \
  buffer = (unsigned char*) myRealloc((void *) buffer, bufferSize, bufferSize + x); \
  memcpy(&buffer[bufferSize], readBuffer, x); \
  bufferSize += x; \
}

enum Phase { CALIBRATE, SYNCHRONIZE, INIT, READ_LENGTH, READ_DATA, READ_CRC, VERIFY, EXIT };

PROCESS(light_app_process, "light app process");
AUTOSTART_PROCESSES(&light_app_process);

// CALIBRATE
int window[KMEANS_VALUES];
int recorded = 0, threshold = -1;
KMeans kmeans;

// SYNCHRONIZE
int periodsMeasured = 0;
unsigned char lastSyncValue = 255;
clock_time_t syncStartTime, periodLength;

// INIT
unsigned char INIT_PATTERN = 'k'; // 01101011
unsigned char INIT_PATTERN_HAMMING = 'h';

unsigned char last8bits = '\0';

// READ
char useHamming = 0;
unsigned char dataBuffer[16];
unsigned char curChar = '\0';
int bitsRead = 0;

enum Phase phase = CALIBRATE;

void activateLED(unsigned char ledv) {
  leds_off(LEDS_ALL);
  leds_on(ledv);
}

int myRound(float num) {
  return (int) (num + 0.5);
}

int myMin(int a, int b) {
    return a<b ? a : b;
}

void *myRealloc(void * buffer, unsigned long bufferSize, size_t size) {
  void * newBuffer = malloc(size);
  memcpy(newBuffer, buffer, myMin(size, bufferSize));
  return newBuffer;
}

void calibrate(int newValue) {
  int i;

  if (recorded < KMEANS_VALUES) {
    PRINTF("%d ", newValue);
    window[recorded] = newValue;
    recorded++;
    return;
  }

  buildClusters(window, KMEANS_VALUES, K_CLUSTERS, &kmeans);

  phase = SYNCHRONIZE;

  PRINTF("\nCalibration finished, cluster means are:");
  for (i = 0; i < kmeans.k; i++) {
    PRINTF(" %d", kmeans.centers[i]);
  }
  PRINTF("\n");
}

unsigned char getBinaryValue(int intValue) {
  return classify(intValue, &kmeans);
}

void synchronize(int value) {
  unsigned char syncValue = getBinaryValue(value);

  if (lastSyncValue != 255 && syncValue != lastSyncValue) {
    if (syncStartTime) {
      periodsMeasured += 1;
      if (periodsMeasured == 20) {
        periodLength = clock_time() - syncStartTime;
        periodLength = myRound((float) periodLength / 20);

        PRINTF("Synchronization finished, periodLength is %d clock ticks, which is %lums\n\n", (int) periodLength,
          ((long) periodLength) * 1000 / CLOCK_SECOND);
        phase = INIT;
      }
    } else {
      syncStartTime = clock_time();
    }
  }

  lastSyncValue = syncValue;
}

void init(int value) {
  unsigned char initValue = getBinaryValue(value);

  last8bits = last8bits << 1;
  last8bits += initValue;

  PRINTF("%d", initValue);

  if (last8bits == INIT_PATTERN || last8bits == INIT_PATTERN_HAMMING) {
    if (last8bits == INIT_PATTERN_HAMMING) useHamming = 1;
    // check here, if we received the init patter for active hamming code
    // and set bitsToRead accordingly
    PRINTF("\nInitialization finished\n\n");
    activateLED(LEDS_BLUE);
    phase = READ_LENGTH;
  }
}

int read(int value, unsigned char* readBuffer, unsigned int readBufferBytesRead, unsigned int printChars) {
  int hammingError1, hammingError2;
  unsigned char charBits[8];
  unsigned char readChar;
  unsigned char bitValue = getBinaryValue(value);

  dataBuffer[bitsRead] = bitValue;
  bitsRead++;
  if (useHamming && bitsRead == 16) {
    // detectAndCorrectError just uses the first 8 elements of the given array
    hammingError1 = detectAndCorrectError(dataBuffer);
    // give it a pointer to the second half of the array
    hammingError2 = detectAndCorrectError(&dataBuffer[8]);

    if (hammingError1 == TWO_BIT_ERROR || hammingError2 == TWO_BIT_ERROR) {
      // error cannot be corrected
      PRINTF("\nError while transmission, detected by hamming code\n");
      activateLED(LEDS_RED);
      phase = EXIT;
    } else if (hammingError1 == ONE_BIT_ERROR || hammingError2 == ONE_BIT_ERROR) {
      // error was corrected
    }

    // decode the hamming codes into the bits of the transmitted character
    decode(dataBuffer, charBits);
    decode(&dataBuffer[8], &charBits[4]);
    readChar = binaryStringToASCII(charBits);

    if (printChars == 1) { PRINTF("%c", readChar); }
    readBuffer[readBufferBytesRead] = readChar;
    bitsRead = 0;
    return 1;
  } else if (!useHamming && bitsRead == 8) {
    readChar = binaryStringToASCII(dataBuffer);
    if (printChars == 1) { PRINTF("%c", readChar); }
    readBuffer[readBufferBytesRead] = readChar;
    bitsRead = 0;
    return 1;
  }

  return 0;
}

unsigned char binaryStringToASCII(const unsigned char* binaryString) {
  // this method uses the first 8 elements of binaryString and converts them to a char
  int j, bitsPerChar = 8;
  unsigned char charNum = 0;

  for (j = 0; j < bitsPerChar; j++) {
    charNum += binaryString[j] << (bitsPerChar - j - 1);
  }

  return charNum;
}

int verify(unsigned char* buffer, unsigned long bufferSize, unsigned long dataLength) {
  unsigned long crcSumApp, crcSumMote;
  unsigned char* data = malloc(dataLength);

  // Copy data and crcSumApp from overall buffer into seperate ones
  memcpy(data, &buffer[4], dataLength);
  memcpy(&crcSumApp, &buffer[bufferSize - 4], 4);

  crcSumMote = crc32(data, dataLength);

  PRINTF("\nChecksum mote: %lu ", crcSumMote);
  PRINTF("\nChecksum app: %lu ", crcSumApp);
  return crcSumMote == crcSumApp;
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(light_app_process, ev, data)
{
  static struct etimer et;
  static int i;

  static unsigned char* buffer;
  static size_t bufferSize = 0;

  static char initializedBufferLength = 0, initializedBufferData = 0, initializedBufferCrc = 0;
  static unsigned long dataLength;
  static unsigned char* readBuffer;
  static unsigned int readBufferBytesRead;

  PROCESS_BEGIN();

  SENSORS_ACTIVATE(light_sensor);

  PRINTF("Countdown for calibration...");
  i = 3;
  for (i = 3; i >= 0; i--) {
    etimer_set(&et, CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    PRINTF(" %d...", i);
    // Activate LEDs in the following order: Red, Blue, Green
    activateLED(1 << (i-1));
  }
  PRINTF("go!\n\nCalibration values:\n");

  while(phase != EXIT) {

    int value = light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR);

    int waitTime = 0;
    if (phase == CALIBRATE) {
      waitTime = CLOCK_SECOND / CAPTURE_FREQUENCY;
      etimer_set(&et, waitTime);
      calibrate(value);
    } else if (phase == SYNCHRONIZE) {
      waitTime = 1;
      etimer_set(&et, waitTime);
      int startTime = clock_time();
      synchronize(value);
      if (phase == INIT) {
        int execTime = clock_time() - startTime;
        waitTime = periodLength - 2 - execTime;
        etimer_set(&et, waitTime);
      }
    } else if (phase == INIT) {
      waitTime = periodLength;
      etimer_set(&et, waitTime);
      init(value);
    } else if (phase == READ_LENGTH) {
      waitTime = periodLength;
      etimer_set(&et, waitTime);
      READ_X_BYTES(4, Length, READ_DATA, 0);
    } else if (phase == READ_DATA) {
      if (initializedBufferData == 0) {
        memcpy(&dataLength, buffer, 4);
        PRINTF("Data length: %lu \n", dataLength);
      }
      waitTime = periodLength;
      etimer_set(&et, waitTime);
      READ_X_BYTES(dataLength, Data, READ_CRC, 1);
    } else if (phase == READ_CRC) {
      waitTime = periodLength;
      etimer_set(&et, waitTime);
      READ_X_BYTES(4, Crc, VERIFY, 0);
    } else if (phase == VERIFY) {
      // wait x seconds before terminating
      waitTime = 10 * CLOCK_SECOND;
      etimer_set(&et, waitTime);

      int dataCorrect = verify(buffer, bufferSize, dataLength);
      // activate LEDS_GREEN or LEDS_RED based on return value
      if (dataCorrect) {
        activateLED(LEDS_GREEN);
      } else {
        activateLED(LEDS_RED);
      }
      phase = EXIT;
    }

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    // PRINTF("light %d %d\n", light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC),
        // light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR));
  }

  leds_off(LEDS_ALL);

  // this memory was allocated in buildClusters and needs to be freed
  free(kmeans.centers);
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/