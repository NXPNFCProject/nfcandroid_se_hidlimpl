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
#ifndef VENDOR_NXP_NXPESE_V1_0_NXPWIREDSE_H
#define VENDOR_NXP_NXPESE_V1_0_NXPWIREDSE_H

#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <vendor/nxp/nxpese/1.0/INxpWiredSe.h>

namespace vendor {
namespace nxp {
namespace nxpese {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_death_recipient;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using ::android::wp;

struct NxpWiredSe : public INxpWiredSe, public hidl_death_recipient {
  // Methods from ::vendor::nxp::nxpese::V1_0::INxpWiredSe follow.
  Return<void> setWiredSeCallback(
      const sp<::vendor::nxp::nxpese::V1_0::INxpWiredSeHalCallback>&
          wiredCallback) override;
  void serviceDied(uint64_t /*cookie*/, const wp<IBase>& /*who*/) override;
  static android::sp<INxpWiredSeHalCallback> sNxpWiredCallbackHandle;
  // Methods from ::android::hidl::base::V1_0::IBase follow.
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace nxpese
}  // namespace nxp
}  // namespace vendor

#endif  // VENDOR_NXP_NXPESE_V1_0_NXPWIREDSE_H
