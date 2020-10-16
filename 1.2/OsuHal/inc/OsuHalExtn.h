/******************************************************************************
 *
 *  Copyright 2020 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#pragma once
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <hardware/hardware.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <string.h>
#include "phNxpEse_Api.h"

using ::android::hardware::hidl_vec;
#define ISO7816_CLA_CHN_MASK 0x03
#define OSU_PROP_CLA 0x80
#define OSU_PROP_INS 0xDF
#define OSU_PROP_RST_P1 0xEF
#define ISO7816_BASIC_CHANNEL 0x00

#define IS_OSU_MODE(...) (OsuHalExtn::getInstance().isOsuMode(__VA_ARGS__))

class OsuHalExtn {
 public:
  typedef enum {
    NON_OSU_MODE,
    OSU_PROP_MODE,
    OSU_GP_MODE,
    OSU_RST_MODE,
  } OsuApduMode;

  typedef enum {
    INIT,
    OPENBASIC,
    OPENLOGICAL,
    TRANSMIT,
    CLOSE,
    RESET,
  } SecureElementAPI;

  static OsuHalExtn& getInstance();

  OsuApduMode isOsuMode(const hidl_vec<uint8_t>& evt, uint8_t type,
                        phNxpEse_data* pCmdData = nullptr);
  bool isOsuMode(uint8_t type, uint8_t channel = 0xFF);
  virtual ~OsuHalExtn();
  OsuHalExtn() noexcept;
  void checkAndUpdateOsuMode();
  unsigned long int getOSUMaxWtxCount();

 private:
  bool isAppOSUMode;
  bool isJcopOSUMode;
  static const hidl_vec<uint8_t> osu_aid[10];
  OsuApduMode checkTransmit(uint8_t* input, size_t length, uint32_t* outLength);
  bool isOsuMode();
};
