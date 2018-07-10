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
#define LOG_TAG "NxpWiredEseHal"
#include "NxpWiredSe.h"
#include <WiredSe.h>
#include <log/log.h>

namespace vendor {
namespace nxp {
namespace nxpese {
namespace V1_0 {
namespace implementation {
using vendor::nxp::wired_se::V1_0::implementation::WiredSe;
android::sp<INxpWiredSeHalCallback> NxpWiredSe::sNxpWiredCallbackHandle = nullptr;
// Methods from ::vendor::nxp::nxpese::V1_0::INxpNxpWiredSe follow.
Return<void> NxpWiredSe::setWiredSeCallback(
    const android::sp< ::vendor::nxp::nxpese::V1_0::INxpWiredSeHalCallback>&
        wiredCallback) {
  if (wiredCallback != nullptr) {
    wiredCallback->linkToDeath(this, 0 /*cookie*/);
  }
  sNxpWiredCallbackHandle = wiredCallback;
  WiredSe::setWiredSeCallback(wiredCallback);
  return Void();
}

void NxpWiredSe::serviceDied(uint64_t /*cookie*/, const wp<IBase>& /*who*/) {
  ALOGE("%s: Enter", __func__);
  if (sNxpWiredCallbackHandle != nullptr) {
    WiredSe::setWiredSeCallback(nullptr);
    sNxpWiredCallbackHandle->unlinkToDeath(this);
  }
}
// Methods from ::android::hidl::base::V1_0::IBase follow.

}  // namespace implementation
}  // namespace V1_0
}  // namespace nxpese
}  // namespace nxp
}  // namespace vendor
