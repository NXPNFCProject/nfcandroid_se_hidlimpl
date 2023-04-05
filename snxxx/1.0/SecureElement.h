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
#ifndef ANDROID_HARDWARE_SECURE_ELEMENT_V1_0_SECUREELEMENT_H
#define ANDROID_HARDWARE_SECURE_ELEMENT_V1_0_SECUREELEMENT_H

#include <android-base/stringprintf.h>
#include <android/hardware/secure_element/1.0/ISecureElement.h>
#include <hardware/hardware.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <pthread.h>

#include "phNxpEse_Api.h"

class ThreadMutex {
 public:
  ThreadMutex();
  virtual ~ThreadMutex();
  void lock();
  void unlock();
  operator pthread_mutex_t*() { return &mMutex; }

 private:
  pthread_mutex_t mMutex;
};

class AutoThreadMutex {
 public:
  AutoThreadMutex(ThreadMutex& m);
  virtual ~AutoThreadMutex();
  operator ThreadMutex&() { return mm; }
  operator pthread_mutex_t*() { return (pthread_mutex_t*)mm; }

 private:
  ThreadMutex& mm;
};

namespace android {
namespace hardware {
namespace secure_element {
namespace V1_0 {
namespace implementation {

using ::android::sp;
using android::base::StringPrintf;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hidl::base::V1_0::IBase;

#ifndef MIN_APDU_LENGTH
#define MIN_APDU_LENGTH 0x04
#endif
#ifndef DEFAULT_BASIC_CHANNEL
#define DEFAULT_BASIC_CHANNEL 0x00
#endif

struct SecureElement : public ISecureElement, public hidl_death_recipient {
  SecureElement();
  Return<void> init(
      const sp<
          ::android::hardware::secure_element::V1_0::ISecureElementHalCallback>&
          clientCallback) override;
  Return<void> getAtr(getAtr_cb _hidl_cb) override;
  Return<bool> isCardPresent() override;
  Return<void> transmit(const hidl_vec<uint8_t>& data,
                        transmit_cb _hidl_cb) override;
  Return<void> openLogicalChannel(const hidl_vec<uint8_t>& aid, uint8_t p2,
                                  openLogicalChannel_cb _hidl_cb) override;
  Return<void> openBasicChannel(const hidl_vec<uint8_t>& aid, uint8_t p2,
                                openBasicChannel_cb _hidl_cb) override;
  Return<::android::hardware::secure_element::V1_0::SecureElementStatus>
  closeChannel(uint8_t channelNumber) override;

  void serviceDied(uint64_t /*cookie*/, const wp<IBase>& /*who*/);

 private:
  uint8_t mMaxChannelCount;
  uint8_t mOpenedchannelCount = 0;
  bool mIsEseInitialized = false;
  static std::vector<bool> mOpenedChannels;
  static sp<V1_0::ISecureElementHalCallback> mCallbackV1_0;
  Return<::android::hardware::secure_element::V1_0::SecureElementStatus>
  seHalDeInit();
  ESESTATUS seHalInit();
  Return<::android::hardware::secure_element::V1_0::SecureElementStatus>
  internalCloseChannel(uint8_t channelNumber);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace secure_element
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_SECURE_ELEMENT_V1_0_SECUREELEMENT_H