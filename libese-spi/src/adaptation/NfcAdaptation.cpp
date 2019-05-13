/******************************************************************************
 *
 *  Copyright 2018-2019 NXP
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

#define LOG_TAG "SpiAddaptation"
#include "NfcAdaptation.h"
#include <android/hardware/nfc/1.0/types.h>
#include <hwbinder/ProcessState.h>
#include <log/log.h>
#include <pthread.h>

using android::sp;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::hidl_vec;
using vendor::nxp::nxpnfc::V1_0::INxpNfc;
using ::android::hardware::hidl_death_recipient;
using ::android::wp;
using ::android::hidl::base::V1_0::IBase;

Mutex NfcAdaptation::sLock;
Mutex NfcAdaptation::sIoctlLock;

sp<INxpNfc> NfcAdaptation::mHalNxpNfc = nullptr;
NfcAdaptation *NfcAdaptation::mpInstance = nullptr;

int omapi_status;
extern bool ese_debug_enabled;

class NxpNfcDeathRecipient : public hidl_death_recipient {
 public:
  sp<INxpNfc> mHalNxpNfcDeathRsp;
  NxpNfcDeathRecipient(sp<INxpNfc>& mHalNxpNfc) {
    mHalNxpNfcDeathRsp = mHalNxpNfc;
  }
  virtual void serviceDied(
      uint64_t /* cookie */,
      const wp<::android::hidl::base::V1_0::IBase>& /* who */) {
    ALOGE("NxpNfcDeathRecipient::serviceDied - Nfc HalService died");
    mHalNxpNfcDeathRsp->unlinkToDeath(this);
    mHalNxpNfcDeathRsp = NULL;
    NfcAdaptation::GetInstance().resetNxpNfcHalReference();
  }
};

/*******************************************************************************
**
** Function:    NfcAdaptation::Initialize()
**
** Description: Tries to get reference to Hw service
**
** Returns:     none
**
*******************************************************************************/
void NfcAdaptation::Initialize() {
  const char* func = "NfcAdaptation::Initialize";
  ALOGD_IF(ese_debug_enabled, "%s", func);
  if (mHalNxpNfc != nullptr) return;
  resetNxpNfcHalReference();
  ALOGD_IF(ese_debug_enabled, "%s: exit", func);
}

/*******************************************************************************
**
** Function:    NfcAdaptation::resetNxpNfcHalReference()
**
** Description: Resets and gets the new hardware service reference
**
** Returns:     none
**
*******************************************************************************/
void NfcAdaptation::resetNxpNfcHalReference() {
  mHalNxpNfc = nullptr;
  for (int cnt = 0; ((mHalNxpNfc == nullptr) && (cnt < 3)); cnt++) {
    mHalNxpNfc = INxpNfc::tryGetService();
    LOG_FATAL_IF(mHalNxpNfc == nullptr, "Failed to retrieve the NXP NFC HAL!");
    if (mHalNxpNfc != nullptr) {
      ALOGD_IF(ese_debug_enabled, "%s: INxpNfc::getService() returned %p (%s)",
               __func__, mHalNxpNfc.get(),
               (mHalNxpNfc->isRemote() ? "remote" : "local"));
      mNxpNfcDeathRecipient = new NxpNfcDeathRecipient(mHalNxpNfc);
      mHalNxpNfc->linkToDeath(mNxpNfcDeathRecipient, 0);
    } else {
      usleep(100 * 1000);
    }
  }
}

/*******************************************************************************
**
** Function:    NfcAdaptation::GetInstance()
**
** Description: access class singleton
**
** Returns:     pointer to the singleton object
**
*******************************************************************************/
NfcAdaptation& NfcAdaptation::GetInstance() {
  AutoMutex guard(sLock);

  if (!mpInstance) mpInstance = new NfcAdaptation;
  return *mpInstance;
}

/*******************************************************************************
**
** Function:    NfcAdaptation::NfcAdaptation()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
NfcAdaptation::NfcAdaptation() { mCurrentIoctlData = NULL; }

/*******************************************************************************
**
** Function:    NfcAdaptation::~NfcAdaptation()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
NfcAdaptation::~NfcAdaptation() { mpInstance = NULL; }

/*******************************************************************************
**
** Function:    IoctlCallback
**
** Description: Callback from HAL stub for IOCTL api invoked.
**              Output data for IOCTL is sent as argument
**
** Returns:     None.
**
*******************************************************************************/
void IoctlCallback(::android::hardware::nfc::V1_0::NfcData outputData) {
  const char* func = "IoctlCallback";
  ese_nxp_ExtnOutputData_t* pOutData =
      (ese_nxp_ExtnOutputData_t*)&outputData[0];
  ALOGD_IF(ese_debug_enabled, "%s Ioctl Type=%lu", func,
           (unsigned long)pOutData->ioctlType);
  NfcAdaptation* pAdaptation = (NfcAdaptation*)pOutData->context;
  /*Output Data from stub->Proxy is copied back to output data
   * This data will be sent back to libnfc*/
  memcpy(&pAdaptation->mCurrentIoctlData->out, &outputData[0],
         sizeof(ese_nxp_ExtnOutputData_t));
  ALOGD_IF(ese_debug_enabled, "%s Ioctl Type value[0]:0x%x and value[3] 0x%x",
           func, pOutData->data.nxpRsp.p_rsp[0],
           pOutData->data.nxpRsp.p_rsp[3]);
  omapi_status = pOutData->data.nxpRsp.p_rsp[3];
}

/*******************************************************************************
**
** Function:    NfcAdaptation::HalIoctl
**
** Description: Calls ioctl to the Nfc driver.
**              If called with a arg value of 0x01 than wired access requested,
**              status of the requst would be updated to p_data.
**              If called with a arg value of 0x00 than wired access will be
**              released, status of the requst would be updated to p_data.
**              If called with a arg value of 0x02 than current p61 state would
*be
**              updated to p_data.
**
** Returns:     -1 or 0.
**
*******************************************************************************/
ESESTATUS NfcAdaptation::HalIoctl(long arg, void* p_data) {
  const char* func = "NfcAdaptation::HalIoctl";
  ::android::hardware::nfc::V1_0::NfcData data;
  ESESTATUS result = ESESTATUS_FAILED;
  AutoMutex guard(sIoctlLock);
  ese_nxp_IoctlInOutData_t* pInpOutData = (ese_nxp_IoctlInOutData_t*)p_data;
  ALOGD_IF(ese_debug_enabled, "%s arg=%ld", func, arg);
  pInpOutData->inp.context = &NfcAdaptation::GetInstance();
  NfcAdaptation::GetInstance().mCurrentIoctlData = pInpOutData;
  data.setToExternal((uint8_t*)pInpOutData, sizeof(ese_nxp_IoctlInOutData_t));
  if (mHalNxpNfc != nullptr) {
    mHalNxpNfc->ioctl(arg, data, IoctlCallback);
  }
  ALOGD_IF(ese_debug_enabled, "%s Ioctl Completed for Type=%lu", func,
           (unsigned long)pInpOutData->out.ioctlType);
  result = (ESESTATUS)(pInpOutData->out.result);
  return result;
}
