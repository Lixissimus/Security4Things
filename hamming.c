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

#include "hamming.h"
#include <stdio.h>
#define ROWS 4
#define COLS 8

int detectAndCorrectError(int* codeword) {
 	// Return 0 if there was no error or if it was corrected, non-zero otherwise.
 	// Changes codeword array!
 	// Codeword should have structure: P0 P1 D0 P2 D1 D2 D3 P3
 	int i, j, error, parity;
 	int prod[ROWS];
	const int controlMatrix[ROWS][COLS] = {
		{1, 0, 1, 0, 1, 0, 1, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 0, 0, 1, 1, 1, 1, 0},
		{1, 1, 1, 1, 1, 1, 1, 1}
	};

	for (i = 0; i < ROWS; i++) {
		prod[i] = 0;
		for (j = 0; j < COLS; j++) {
			prod[i] += controlMatrix[i][j] * codeword[j];
		}
		prod[i] = prod[i] % 2;
	}

	// convert prod-vector into int
	error = prod[2]*4 + prod[1]*2 + prod[0];
	// separate parity bit
	parity = prod[3];

	if (error == 0 && parity == 0) {
		// everything was transmitted correctly, or no error can be detected
		return NO_BIT_ERROR;
	} else if (error != 0 && parity == 1) {
		// error can be corrected by negation 
		codeword[error-1] = codeword[error-1] ^ 1;
		return ONE_BIT_ERROR;
	} else if (error == 0 && parity == 1) {
		// error in parity bit of codeword
		codeword[7] = codeword[7] ^ 1;
		return ONE_BIT_ERROR;
	} else {
		// (error !=0 && parity == 0)
		// 2 bit error, cannot be corrected
		return TWO_BIT_ERROR;
	}
}

void main(int argc, char* argv) {
	int test[8] = {1,0,1,0,0,0,1,1};
	int res = detectAndCorrectError(test);
	int i;
	if (res == TWO_BIT_ERROR) printf("cannot correct error\n");
	for (i = 0; i < 8; i++) {
		printf("%d", test[i]);
	}
	printf("\n");
}