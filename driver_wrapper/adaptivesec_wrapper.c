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

#include "sys/key-flash.h"
#include <stdio.h>
#include "../light-app.h"
#include "adaptivesec_wrapper.h"
#include "net/llsec/adaptivesec/adaptivesec.h"

//#define RESET_KEY 1

static int initializedKey = 0;
static int checkedInitializedKey = 0;
static int keyInitializationTriggered = 0;

/*
 * Checks whether key is initialized and triggers key initialization if necessary.
 * Returns 1 if key is initialized and 0 otherwise.
 */
static int keyInitialization() {
  if (checkedInitializedKey == 0) {
    #ifdef RESET_KEY
    key_flash_erase_keying_material();
    #endif
    key_flash_restore_keying_material(&initializedKey, 1, AES_128_KEY_LENGTH);
    if (initializedKey != 1) initializedKey = 0;
    checkedInitializedKey = 1;
  }

  // Check whether key was already initialized
  if (initializedKey == 1) {
    return 1;
  }

  // Check whether key initialization was already triggered
  if (keyInitializationTriggered == 0) {
    printf("[Adaptivesec_Driver_Wrapper] Trigger key initialization.\n");
    // Trigger light app
    process_start(&light_app_process, NULL);

    keyInitializationTriggered = 1;
    return 0;
  }

  // Check whether key was initialized
  uint8_t initialized = 0;
  key_flash_restore_keying_material(&initialized, 1, AES_128_KEY_LENGTH);
  if (initialized == 1) {
    printf("[Adaptivesec_Driver_Wrapper] Key is initialized.\n");
    initializedKey = 1;
    // Causes a recursive call to keyInitialization, which is fine as initializedKey is now 1
    init();
    return 1;
  }

  // Key initialization was already triggered but key is not initialized yet
  return 0;
}

static void init(void)
{
  if (keyInitialization() == 1) {
    adaptivesec_driver.init();
  }
}

static void send(mac_callback_t sent, void *ptr)
{
  if (keyInitialization() == 1) {
    adaptivesec_driver.send(sent, ptr);
  }
}

static void input(void)
{
  if (keyInitialization() == 1) {
    adaptivesec_driver.input();
  }
}


const struct llsec_driver adaptivesec_driver_wrapper = {
  "adaptivesec_wrapper",
  init,
  send,
  input
};