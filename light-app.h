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

#ifndef LIGHT_APP_H_
#define LIGHT_APP_H_

#ifdef NETSTACK_CONF_LLSEC
#undef NETSTACK_CONF_LLSEC
#endif /* NETSTACK_CONF_LLSEC */
#define NETSTACK_CONF_LLSEC adaptivesec_driver_wrapper

#include "contiki.h"

PROCESS_NAME(light_app_process);

/*
 * Deactivates all other LEDs and activates the ones in the parameter.
 */
void activateLED(unsigned char);

/*
 * Converts a string consisting of "0"s and "1"s to an ascii char.
 * i.e. "01000011" -> 'C'
 */
unsigned char binaryStringToASCII(const unsigned char*);

/*
 * Gathers values from the light sensor and computes their mean value which
 * is then used as a threshold to distinguish between 0 and 1 bits.
 */
void calibrate(int);

/*
 * Returns the binary value for a value from the light sensor according to the
 * threshold that was computed during the calibration.
 */
unsigned char getBinaryValue(int);

/*
 * Detects the first bit switch and computes the time difference to the next
 * bit switch, which is the time each bit is transmitted for. Saves the
 * time of a bit switch in syncStartTime and the transmission time in periodLength.
 */
void synchronize(int);

/*
 * Detects the init pattern, to know when the data is being sent.
 */
void initialize(int);

/*
 * Does stuff with the data.
 */
int read(int, unsigned char*, unsigned int, unsigned int);

/*
 * Computes the crc32 checksum of the data and compares it to the one sent by the client.
 */
int verify(unsigned char*, unsigned long, unsigned long);

#endif /* LIGHT_APP_H_ */