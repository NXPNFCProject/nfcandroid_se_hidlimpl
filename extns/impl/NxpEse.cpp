/******************************************************************************
 *
 *  Copyright (C) 2018 NXP Semiconductors
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

#define LOG_TAG "vendor.nxp.nxpese@1.0-impl"
#include "NxpEse.h"
#include "phNxpEse_Api.h"
#include <log/log.h>
#include "eSEClient.h"

namespace vendor {
namespace nxp {
namespace nxpese {
namespace V1_0 {
namespace implementation {
using ::android::hardware::hidl_vec;
// Methods from ::vendor::nxp::nxpese::V1_0::INxpEse follow.

Return<void> NxpEse::ioctlHandler(uint64_t ioctlType,
                           ese_nxp_IoctlInOutData_t inpOutData) {
  switch(ioctlType)
  {
    case HAL_ESE_IOCTL_NFC_JCOP_DWNLD:
    {
      //nfc_nci_IoctlInOutData_t* inpOutData = (nfc_nci_IoctlInOutData_t*)inpOutData;
      int update_state = inpOutData.inp.data.nxpCmd.p_cmd[0];
      if(update_state == ESE_JCOP_UPDATE_COMPLETED ||
        update_state == ESE_LS_UPDATE_COMPLETED) {
        seteSEClientState(update_state);
        eSEClientUpdate_SE_Thread();
      }
    }
    break;
  }
    return Void();
}

Return<void> NxpEse::ioctl(uint64_t ioctlType,
                           const hidl_vec<uint8_t>& inOutData,
                           ioctl_cb _hidl_cb) {
  ALOGD("NxpEse::ioctl(): enter");
  ese_nxp_IoctlInOutData_t inpOutData;
  memset(&inpOutData, 0, sizeof(inpOutData));
  ese_nxp_IoctlInOutData_t* pInOutData =
      (ese_nxp_IoctlInOutData_t*)&inOutData[0];

  /*data from proxy->stub is copied to local data which can be updated by
   * underlying HAL implementation since its an inout argument*/
  if(ioctlType == HAL_ESE_IOCTL_GET_ESE_UPDATE_STATE) {
    memcpy(&inpOutData, pInOutData, sizeof(ese_nxp_IoctlInOutData_t));
  } else {
    inpOutData.inp.data.nxpCmd.cmd_len = inOutData.size();
    memcpy(&inpOutData.inp.data.nxpCmd.p_cmd, pInOutData,
           inpOutData.inp.data.nxpCmd.cmd_len);
  }
  ESESTATUS status = phNxpEse_spiIoctl(ioctlType, &inpOutData);
  ioctlHandler(ioctlType, inpOutData);

  /*copy data and additional fields indicating status of ioctl operation
   * and context of the caller. Then invoke the corresponding proxy callback*/
  inpOutData.out.ioctlType = ioctlType;
  inpOutData.out.context = pInOutData->inp.context;
  inpOutData.out.result = status;
  if(ioctlType == HAL_ESE_IOCTL_GET_ESE_UPDATE_STATE) {
    inpOutData.out.data.status = (getJcopUpdateRequired() | (getLsUpdateRequired() << 8));
  }
  hidl_vec<uint8_t> outputData;
  outputData.setToExternal((uint8_t*)&inpOutData.out,
                           sizeof(ese_nxp_ExtnOutputData_t));
  ALOGD("GET ESE update state = %d",inpOutData.out.data.status);
  _hidl_cb(outputData);
  ALOGD("NxpEse::ioctl(): exit");
  return Void();
}

Return<void> NxpEse::nfccNtf(uint64_t ntfType,
                             const hidl_vec<uint8_t> &ntfData) {
  ALOGD("NxpEse::nfccNtf(): enter");
  ese_nxp_IoctlInOutData_t inpOutData;
  ese_nxp_IoctlInOutData_t *pInOutData =
      (ese_nxp_IoctlInOutData_t *)&ntfData[0];
  /*data from proxy->stub is copied to local data*/
  memcpy(&inpOutData, pInOutData, sizeof(ese_nxp_IoctlInOutData_t));
  phNxpEse_spiIoctl(ntfType, &inpOutData);
  ALOGD("NxpEse::nfccNtf(): exit");
  return Void();
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

}  // namespace implementation
}  // namespace V1_0
}  // namespace nxpese
}  // namespace nxp
}  // namespace vendor
