/******************************************************************************
 *
 *  Copyright 2018 NXP
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
#include <pthread.h>

#include "SyncEvent.h"
#include "hal_nxpnfc.h"
#include "hal_nxpese.h"
#include <android/hardware/nfc/1.0/types.h>
#include <phEseStatus.h>
#include <utils/RefBase.h>
#include <vendor/nxp/nxpnfc/1.0/INxpNfc.h>

using vendor::nxp::nxpnfc::V1_0::INxpNfc;
using ::android::sp;
class NxpNfcDeathRecipient;

class NfcAdaptation {
 public:
   ~NfcAdaptation();
   void Initialize();
   static NfcAdaptation &GetInstance();
   static ESESTATUS HalIoctl(long data_len, void *p_data);
   void resetNxpNfcHalReference();
   ese_nxp_IoctlInOutData_t *mCurrentIoctlData;

 private:
  NfcAdaptation();
  static Mutex sLock;
  static Mutex sIoctlLock;
  static NfcAdaptation *mpInstance;
  static android::sp<INxpNfc> mHalNxpNfc;
  sp<NxpNfcDeathRecipient> mNxpNfcDeathRecipient;
};
