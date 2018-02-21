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
#define LOG_TAG "SecElement_hidl_hal_test"
#include <android-base/logging.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>

#include <android/hardware/secure_element/1.0/ISecureElement.h>
#include <android/hardware/secure_element/1.0/ISecureElementHalCallback.h>
#include <android/hardware/secure_element/1.0/types.h>

using ::android::hardware::secure_element::V1_0::ISecureElement;
using ::android::hardware::secure_element::V1_0::ISecureElementHalCallback;
using ::android::hardware::secure_element::V1_0::SecureElementStatus;
using ::android::hardware::secure_element::V1_0::LogicalChannelResponse;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using android::hardware::hidl_vec;

#define APDU_DATA \
  { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }

#define SELECT_APDU_DATA \
  { 0x6C, 0x6F, 0x6F, 0x70, 0x62, 0x61, 0x63, 0x6B, 0x41 }
class SeclementClientCallbackArgs {
 public:
  bool card_present_;
};

class SecureElementCallback : public ::testing::VtsHalHidlTargetCallbackBase<
                                  SeclementClientCallbackArgs>,
                              public ISecureElementHalCallback {
 public:
  virtual ~SecureElementCallback() = default;

  Return<void> onStateChange(bool cardPresent) override {
    LOG(INFO) << " onStateChange Callback received";
    SeclementClientCallbackArgs args;
    args.card_present_ = cardPresent;
    return Void();
  };
};

// The main test class for SecureElement HIDL HAL.
class SecElementHidlTest : public ::testing::VtsHalHidlTargetTestBase {
 public:
  virtual void SetUp() override {
    se_ =
        ::testing::VtsHalHidlTargetTestBase::getService<ISecureElement>("eSE1");
    se_cb_ = new SecureElementCallback();
    ASSERT_NE(se_, nullptr);
    LOG(INFO) << " SetUp Done !!";
  }

  virtual void TearDown() override { LOG(INFO) << " TearDown Done !!"; }

  sp<ISecureElement> se_;
  sp<SecureElementCallback> se_cb_;
};

// A class for test environment setup (kept since this file is a template).
class SecElementHidlEnvironment : public ::testing::Environment {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}

 private:
};

TEST_F(SecElementHidlTest, Init) {
  se_->init(se_cb_);
}

TEST_F(SecElementHidlTest, Transmit) {
  hidl_vec<uint8_t> cmd = APDU_DATA;
  se_->transmit(cmd, [&](hidl_vec<uint8_t> result) {
    LOG(INFO) << "Test result size = " << result.size();
  });
}

TEST_F(SecElementHidlTest, openLogicalChannel) {
  hidl_vec<uint8_t> aid = SELECT_APDU_DATA;
  uint8_t p2 = 0x0;

  se_->openLogicalChannel(aid, p2, [&](LogicalChannelResponse result,
                                       SecureElementStatus seStatus) {
    EXPECT_EQ(SecureElementStatus::SUCCESS, seStatus);
    LOG(INFO) << "Channel opened " << (uint8_t)result.channelNumber;
  });
  se_->openLogicalChannel(aid, p2, [&](LogicalChannelResponse result,
                                       SecureElementStatus seStatus) {
    EXPECT_EQ(SecureElementStatus::SUCCESS, seStatus);
    LOG(INFO) << "Channel opened " << (uint8_t)result.channelNumber;
  });
  se_->openLogicalChannel(aid, p2, [&](LogicalChannelResponse result,
                                       SecureElementStatus seStatus) {
    EXPECT_EQ(SecureElementStatus::SUCCESS, seStatus);
    LOG(INFO) << "Channel opened " << (uint8_t)result.channelNumber;
  });
  se_->openLogicalChannel(aid, p2, [&](LogicalChannelResponse result,
                                       SecureElementStatus seStatus) {
    EXPECT_EQ(SecureElementStatus::CHANNEL_NOT_AVAILABLE, seStatus);
    LOG(INFO) << "Channel opened " << (uint8_t)result.channelNumber;
  });
}

TEST_F(SecElementHidlTest, openBasicChannel) {
  hidl_vec<uint8_t> aid = NULL;
  uint8_t p2 = 0;
  se_->openBasicChannel(aid, p2,
                        [&](hidl_vec<uint8_t>, SecureElementStatus seStatus) {
                          EXPECT_EQ(SecureElementStatus::SUCCESS, seStatus);
                        });
  se_->openBasicChannel(aid, p2,
                        [&](hidl_vec<uint8_t>, SecureElementStatus seStatus) {
                          EXPECT_EQ(SecureElementStatus::SUCCESS, seStatus);
                        });
  se_->openBasicChannel(aid, p2,
                        [&](hidl_vec<uint8_t>, SecureElementStatus seStatus) {
                          EXPECT_EQ(SecureElementStatus::SUCCESS, seStatus);
                        });
}

TEST_F(SecElementHidlTest, closeChannel) {
  EXPECT_EQ(SecureElementStatus::SUCCESS, se_->closeChannel(1));
  EXPECT_EQ(SecureElementStatus::SUCCESS, se_->closeChannel(2));
  EXPECT_EQ(SecureElementStatus::SUCCESS, se_->closeChannel(3));
}

int main(int argc, char** argv) {
  ::testing::AddGlobalTestEnvironment(new SecElementHidlEnvironment);
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;
  return status;
}
