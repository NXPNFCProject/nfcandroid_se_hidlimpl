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
#define LOG_TAG "WiredEseHal"
#include "WiredSe.h"
#include <android-base/logging.h>
#include <log/log.h>

#include "NxpEse.h"
#include "hal_nxpese.h"
using vendor::nxp::nxpese::V1_0::implementation::NxpEse;
using android::hardware::secure_element::V1_0::ISecureElementHalCallback;
using vendor::nxp::wired_se::V1_0::implementation::WiredSe;

extern WiredSe * pWiredSe;
namespace vendor {
namespace nxp {
namespace wired_se {
namespace V1_0 {
namespace implementation {
android::sp<INxpWiredSeHalCallback> WiredSe::sWiredCallbackHandle = nullptr;
static hidl_vec<uint8_t> gsRspDataBuff(256);
static android::sp<ISecureElementHalCallback> gSeHalCallback;
std::vector<uint8_t> atrResponse;

WiredSe::WiredSe()
    : mOpenedchannelCount(0),
      mOpenedChannels{false, false, false, false},
      mWiredSeHandle(0) {}

Return<void> WiredSe::init(const sp<
    ::android::hardware::secure_element::V1_0::ISecureElementHalCallback>&
                               clientCallback) {
  if (clientCallback == nullptr) {
    return Void();
  }
  gSeHalCallback = clientCallback;
  gSeHalCallback->linkToDeath(this, 0 /*cookie*/);
  if (sWiredCallbackHandle == nullptr) {
    gSeHalCallback->onStateChange(false);
    ALOGE("%s: NfcService callback handle not registered yet!", __func__);
    return Void();
  }
  ALOGD("%s: Handle registration success", __func__);

  mWiredSeHandle = sWiredCallbackHandle->openWiredSe();
  if (mWiredSeHandle <= 0) {
    ALOGE("%s: openWiredSe failed!", __func__);
    gSeHalCallback->onStateChange(false);
  } else {
    ALOGD("%s: Handle registration success", __func__);
    std::vector<uint8_t> getAtrResponse;
    sWiredCallbackHandle->getAtr(mWiredSeHandle,
                                 [&getAtrResponse](std::vector<uint8_t> res) {
                                   getAtrResponse.resize(res.size());
                                   for (size_t i = 0; i < res.size(); i++) {
                                     getAtrResponse[i] = res[i];
                                   }
                                 });
    atrResponse.swap(getAtrResponse);
    gSeHalCallback->onStateChange(true);
  }
  return Void();
}

Return<void> WiredSe::getAtr(getAtr_cb _hidl_cb) {
  hidl_vec<uint8_t> response;
  if ((!sWiredCallbackHandle) || (mWiredSeHandle <= 0)) {
    ALOGE("%s: Handle to access NfcWiredSe is invalid", __func__);
    _hidl_cb(response);
    return Void();
  }
  _hidl_cb(atrResponse);
  return Void();
}

Return<bool> WiredSe::isCardPresent() { return true; }

Return<void> WiredSe::transmit(const hidl_vec<uint8_t>& data,
                               transmit_cb _hidl_cb) {
  std::vector<uint8_t> transmitResponse;
  if ((!sWiredCallbackHandle) || (mWiredSeHandle <= 0)) {
    ALOGE("%s: Handle to access NfcWiredSe is invalid", __func__);
    _hidl_cb(transmitResponse);
    return Void();
  }
  /* Deep copy of the reponse buffer after transmit */
  sWiredCallbackHandle->transmit(data, mWiredSeHandle,
                                 [&transmitResponse](std::vector<uint8_t> res) {
                                   transmitResponse.resize(res.size());
                                   for (size_t i = 0; i < res.size(); i++) {
                                     transmitResponse[i] = res[i];
                                   }
                                 });
  _hidl_cb(transmitResponse);
  return Void();
}

Return<void> WiredSe::openLogicalChannel(const hidl_vec<uint8_t>& aid,
                                         uint8_t p2,
                                         openLogicalChannel_cb _hidl_cb) {
  std::vector<uint8_t> manageChannelCommand = {0x00, 0x70, 0x00, 0x00, 0x01};

  LogicalChannelResponse resApduBuff;

  resApduBuff.channelNumber = 0xff;
  memset(&resApduBuff, 0x00, sizeof(resApduBuff));
  if (mWiredSeHandle <= 0) {
    WIREDSESTATUS status = seHalInit();
    if (status != WIREDSESTATUS_SUCCESS) {
      ALOGE("%s: seHalInit Failed!!!", __func__);
      _hidl_cb(resApduBuff, SecureElementStatus::IOERROR);
      return Void();
    }
  }

  SecureElementStatus sestatus = SecureElementStatus::IOERROR;
  WIREDSESTATUS status = WIREDSESTATUS_FAILED;
  /* Deep copy of the reponse buffer after transmit */
  std::vector<uint8_t> rspApdu;
  sWiredCallbackHandle->transmit(manageChannelCommand, mWiredSeHandle,
                                 [&rspApdu](std::vector<uint8_t> res) {
                                   rspApdu.resize(res.size());
                                   for (size_t i = 0; i < res.size(); i++) {
                                     rspApdu[i] = res[i];
                                   }
                                 });
  if (rspApdu.size() <= 0)
    status = WIREDSESTATUS_FAILED;
  else
    status = WIREDSESTATUS_SUCCESS;

  if (status != WIREDSESTATUS_SUCCESS) {
    resApduBuff.channelNumber = 0xff;
    if (rspApdu.size() > 0 &&
        (rspApdu.at(0) == 0x64 && rspApdu.at(1) == 0xFF)) {
      sestatus = SecureElementStatus::IOERROR;
    } else {
      sestatus = SecureElementStatus::FAILED;
    }
  } else if (rspApdu.at(rspApdu.size() - 2) == 0x6A &&
             rspApdu.at(rspApdu.size() - 1) == 0x81) {
    resApduBuff.channelNumber = 0xff;
    sestatus = SecureElementStatus::CHANNEL_NOT_AVAILABLE;
  } else if (rspApdu.at(rspApdu.size() - 2) == 0x90 &&
             rspApdu.at(rspApdu.size() - 1) == 0x00) {
    resApduBuff.channelNumber = rspApdu.at(0);
    mOpenedchannelCount++;
    mOpenedChannels[resApduBuff.channelNumber] = true;
    sestatus = SecureElementStatus::SUCCESS;
  } else if (((rspApdu.at(rspApdu.size() - 2) == 0x6E) ||
              (rspApdu.at(rspApdu.size() - 2) == 0x6D)) &&
             rspApdu.at(rspApdu.size() - 1) == 0x00) {
    sestatus = SecureElementStatus::UNSUPPORTED_OPERATION;
  }

  if (sestatus != SecureElementStatus::SUCCESS) {
    if (mOpenedchannelCount == 0) {
      sestatus = seHalDeInit();
    }
    /*If manageChannel is failed in any of above cases
    send the callback and return*/
    _hidl_cb(resApduBuff, sestatus);
    return Void();
  }
  ALOGD("%s: openLogicalChannel Sending selectApdu", __func__);

  sestatus = SecureElementStatus::IOERROR;
  status = WIREDSESTATUS_FAILED;

  std::vector<uint8_t> selectCommand(aid.size() + 5);
  selectCommand.at(0) = resApduBuff.channelNumber;
  selectCommand.at(1) = (uint8_t)0xA4;
  selectCommand.at(2) = 0x04;
  selectCommand.at(3) = p2;
  selectCommand.at(4) = (uint8_t)aid.size();
  std::copy(aid.begin(), aid.end(), &selectCommand.at(5));

  /* Deep copy of the reponse buffer after transmit */
  std::vector<uint8_t> rspSelectApdu;
  sWiredCallbackHandle->transmit(selectCommand, mWiredSeHandle,
                                 [&rspSelectApdu](std::vector<uint8_t> res) {
                                   rspSelectApdu.resize(res.size());
                                   for (size_t i = 0; i < res.size(); i++) {
                                     rspSelectApdu[i] = res[i];
                                   }
                                 });

  if (rspSelectApdu.size() <= 0)
    status = WIREDSESTATUS_FAILED;
  else
    status = WIREDSESTATUS_SUCCESS;

  if (status != WIREDSESTATUS_SUCCESS) {
    /*Transceive failed*/
    if (rspSelectApdu.size() > 0 && (*(rspSelectApdu.end() - 2) == 0x64 &&
                                     *(rspSelectApdu.end() - 1) == 0xFF)) {
      _hidl_cb(resApduBuff, SecureElementStatus::IOERROR);
    } else {
      _hidl_cb(resApduBuff, SecureElementStatus::FAILED);
    }
  } else {
    /*Status is success*/
    if (*(rspSelectApdu.end() - 2) == 0x90 &&
        *(rspSelectApdu.end() - 1) == 0x00) {
      sestatus = SecureElementStatus::SUCCESS;
      resApduBuff.selectResponse.resize(rspSelectApdu.size());
      for (int i=0; i<rspSelectApdu.size(); i++)
        resApduBuff.selectResponse[i]=rspSelectApdu[i];
    }
    /*AID provided doesn't match any applet on the secure element*/
    else if (*(rspSelectApdu.end() - 2) == 0x6A &&
             *(rspSelectApdu.end() - 1) == 0x82) {
      sestatus = SecureElementStatus::NO_SUCH_ELEMENT_ERROR;
    }
    /*Operation provided by the P2 parameter is not permitted by the applet.*/
    else if (*(rspSelectApdu.end() - 2) == 0x6A &&
             *(rspSelectApdu.end() - 1) == 0x86) {
      sestatus = SecureElementStatus::UNSUPPORTED_OPERATION;
    } else {
      sestatus = SecureElementStatus::FAILED;
    }
  }
  if (sestatus != SecureElementStatus::SUCCESS) {
    SecureElementStatus closeChannelStatus =
        internalCloseChannel(resApduBuff.channelNumber);
    if (closeChannelStatus != SecureElementStatus::SUCCESS) {
      ALOGE("%s: closeChannel Failed", __func__);
    } else {
      resApduBuff.channelNumber = 0xff;
    }
  }
  _hidl_cb(resApduBuff, sestatus);
  return Void();
}

Return<void> WiredSe::openBasicChannel(const hidl_vec<uint8_t>& aid, uint8_t p2,
                                       openBasicChannel_cb _hidl_cb) {
  hidl_vec<uint8_t> result;

  if (mWiredSeHandle <= 0) {
    WIREDSESTATUS status = seHalInit();
    if (status != WIREDSESTATUS_SUCCESS) {
      ALOGE("%s: seHalInit Failed!!!", __func__);
      _hidl_cb(result, SecureElementStatus::IOERROR);
      return Void();
    }
  }

  SecureElementStatus sestatus = SecureElementStatus::IOERROR;
  WIREDSESTATUS status = WIREDSESTATUS_FAILED;

  std::vector<uint8_t> selectCommand(aid.size() + 5);
  selectCommand.at(0) = 0x00;
  selectCommand.at(1) = (uint8_t)0xA4;
  selectCommand.at(2) = 0x04;
  selectCommand.at(3) = p2;
  selectCommand.at(4) = (uint8_t)aid.size();
  std::copy(aid.begin(), aid.end(), &selectCommand.at(5));

  /* Deep copy of the reponse buffer after transmit */
  std::vector<uint8_t> rspSelectApdu;
  sWiredCallbackHandle->transmit(selectCommand, mWiredSeHandle,
                                 [&rspSelectApdu](std::vector<uint8_t> res) {
                                   rspSelectApdu.resize(res.size());
                                   for (size_t i = 0; i < res.size(); i++) {
                                     rspSelectApdu[i] = res[i];
                                   }
                                 });

  if (rspSelectApdu.size() <= 0)
    status = WIREDSESTATUS_FAILED;
  else
    status = WIREDSESTATUS_SUCCESS;

  if (status != WIREDSESTATUS_SUCCESS) {
    /*Transceive failed*/
    if (rspSelectApdu.size() > 0 && (*(rspSelectApdu.end() - 2) == 0x64 &&
                                     *(rspSelectApdu.end() - 1) == 0xFF)) {
      _hidl_cb(result, SecureElementStatus::IOERROR);
    } else {
      _hidl_cb(result, SecureElementStatus::FAILED);
    }
  } else {
    /*Status is success*/
    if (*(rspSelectApdu.end() - 2) == 0x90 &&
        *(rspSelectApdu.end() - 1) == 0x00) {
      if (!mOpenedChannels[0]) {
        mOpenedChannels[0] = true;
        mOpenedchannelCount++;
      }
      result = rspSelectApdu;
      sestatus = SecureElementStatus::SUCCESS;
    }
    /*AID provided doesn't match any applet on the secure element*/
    else if (*(rspSelectApdu.end() - 2) == 0x6A &&
             *(rspSelectApdu.end() - 1) == 0x82) {
      sestatus = SecureElementStatus::NO_SUCH_ELEMENT_ERROR;
    }
    /*Operation provided by the P2 parameter is not permitted by the applet.*/
    else if (*(rspSelectApdu.end() - 2) == 0x6A &&
             *(rspSelectApdu.end() - 1) == 0x86) {
      sestatus = SecureElementStatus::UNSUPPORTED_OPERATION;
    } else {
      sestatus = SecureElementStatus::FAILED;
    }
  }

  if (sestatus != SecureElementStatus::SUCCESS) {
    SecureElementStatus closeChannelStatus =
        internalCloseChannel(DEFAULT_BASIC_CHANNEL);
    if (closeChannelStatus != SecureElementStatus::SUCCESS) {
      ALOGE("%s: closeChannel Failed!!!", __func__);
    }
  }
  _hidl_cb(result, sestatus);
  return Void();
}

Return<::android::hardware::secure_element::V1_0::SecureElementStatus>
WiredSe::internalCloseChannel(uint8_t channelNumber) {
  SecureElementStatus sestatus = SecureElementStatus::FAILED;
  WIREDSESTATUS status = WIREDSESTATUS_SUCCESS;
  ALOGD("%s: Enter", __func__);
  if (sWiredCallbackHandle == nullptr || (mWiredSeHandle <= 0)) {
    ALOGD("%s: sWiredCallbackHandle is nullptr. Returning", __func__);
    return SecureElementStatus::FAILED;
  }

  if (channelNumber < DEFAULT_BASIC_CHANNEL ||
      channelNumber >= MAX_LOGICAL_CHANNELS) {
    ALOGE("%s: invalid channel!!! 0x%02x", __func__, channelNumber);

    sestatus = SecureElementStatus::FAILED;
  } else if (channelNumber > DEFAULT_BASIC_CHANNEL) {
    std::vector<uint8_t> closeCommand(5);
    closeCommand.at(0) = channelNumber;
    closeCommand.at(1) = (uint8_t)0x70;
    closeCommand.at(2) = (uint8_t)0x80;
    closeCommand.at(3) = channelNumber;
    closeCommand.at(4) = 0x00;
    std::vector<uint8_t> rspCloseApdu;
    sWiredCallbackHandle->transmit(closeCommand, mWiredSeHandle,
                                   [&rspCloseApdu](std::vector<uint8_t> res) {
                                     rspCloseApdu.resize(res.size());
                                     for (size_t i = 0; i < res.size(); i++) {
                                       rspCloseApdu[i] = res[i];
                                     }
                                   });
    if (rspCloseApdu.size() <= 0)
      status = WIREDSESTATUS_FAILED;
    else
      status = WIREDSESTATUS_SUCCESS;

    if (status != WIREDSESTATUS_SUCCESS) {
      if ((rspCloseApdu.size() > 0) && (*(rspCloseApdu.end() - 2) == 0x64) &&
          (*(rspCloseApdu.end() - 1) == 0xFF)) {
        sestatus = SecureElementStatus::FAILED;
      } else {
        sestatus = SecureElementStatus::FAILED;
      }
    } else {
      if ((*(rspCloseApdu.end() - 2) == 0x90) &&
          (*(rspCloseApdu.end() - 1) == 0x00)) {
        sestatus = SecureElementStatus::SUCCESS;
      } else {
        sestatus = SecureElementStatus::FAILED;
      }
    }
  }
  if ((channelNumber == DEFAULT_BASIC_CHANNEL) ||
      (sestatus == SecureElementStatus::SUCCESS)) {
    if (mOpenedChannels[channelNumber]) {
      mOpenedChannels[channelNumber] = false;
      mOpenedchannelCount--;
    }
  }
  /*If there are no channels remaining close secureElement*/
  if (mOpenedchannelCount == 0) {
    sestatus = seHalDeInit();
  } else {
    sestatus = SecureElementStatus::SUCCESS;
  }
  return sestatus;
}

Return<::android::hardware::secure_element::V1_0::SecureElementStatus>
WiredSe::closeChannel(uint8_t channelNumber) {
  WIREDSESTATUS status = WIREDSESTATUS_SUCCESS;
  SecureElementStatus sestatus = SecureElementStatus::FAILED;
  ALOGD("%s: Enter 0x%02x", __func__, channelNumber);

  if (sWiredCallbackHandle == nullptr || (mWiredSeHandle <= 0)) {
    ALOGD("%s: sWiredCallbackHandle is nullptr. Returning", __func__);
    return SecureElementStatus::FAILED;
  }

  if (channelNumber < DEFAULT_BASIC_CHANNEL ||
      channelNumber >= MAX_LOGICAL_CHANNELS) {
    ALOGD("%s invalid channel!!! %d for %d", __func__, channelNumber,
          mOpenedChannels[channelNumber]);

    sestatus = SecureElementStatus::FAILED;
  } else if (channelNumber > DEFAULT_BASIC_CHANNEL) {
    std::vector<uint8_t> closeCommand(5);
    closeCommand.at(0) = channelNumber;
    closeCommand.at(1) = (uint8_t)0x70;
    closeCommand.at(2) = (uint8_t)0x80;
    closeCommand.at(3) = channelNumber;
    closeCommand.at(4) = 0x00;
    std::vector<uint8_t> rspCloseApdu;
    sWiredCallbackHandle->transmit(closeCommand, mWiredSeHandle,
                                   [&rspCloseApdu](std::vector<uint8_t> res) {
                                     rspCloseApdu.resize(res.size());
                                     for (size_t i = 0; i < res.size(); i++) {
                                       rspCloseApdu[i] = res[i];
                                     }
                                   });
    if (rspCloseApdu.size() <= 0)
      status = WIREDSESTATUS_FAILED;
    else
      status = WIREDSESTATUS_SUCCESS;
    if (status != WIREDSESTATUS_SUCCESS) {
      if ((rspCloseApdu.size() > 0) && (*(rspCloseApdu.end() - 2) == 0x64) &&
          (*(rspCloseApdu.end() - 1) == 0xFF)) {
        sestatus = SecureElementStatus::FAILED;
      } else {
        sestatus = SecureElementStatus::FAILED;
      }
    } else {
      if ((*(rspCloseApdu.end() - 2) == 0x90) &&
          (*(rspCloseApdu.end() - 1) == 0x00)) {
        sestatus = SecureElementStatus::SUCCESS;
      } else {
        sestatus = SecureElementStatus::FAILED;
      }
    }
  }
  if ((channelNumber == DEFAULT_BASIC_CHANNEL) ||
      (sestatus == SecureElementStatus::SUCCESS)) {
    if (mOpenedChannels[channelNumber]) {
      mOpenedChannels[channelNumber] = false;
      mOpenedchannelCount--;
    }
  }
  /*If there are no channels remaining close secureElement*/
  if (mOpenedchannelCount == 0) {
    sestatus = seHalDeInit();
  } else {
    sestatus = SecureElementStatus::SUCCESS;
  }
  return sestatus;
}

void WiredSe::serviceDied(uint64_t /*cookie*/, const wp<IBase>& /*who*/) {
  ALOGE("%s: WiredSe serviceDied!!!", __func__);
  SecureElementStatus sestatus = seHalDeInit();
  if (sestatus != SecureElementStatus::SUCCESS) {
    ALOGE("%s: seHalDeInit Faliled!!!", __func__);
  }
  if (gSeHalCallback != nullptr) {
    gSeHalCallback->unlinkToDeath(this);
  }
}

WIREDSESTATUS WiredSe::seHalInit() {
  WIREDSESTATUS status = WIREDSESTATUS_SUCCESS;
  if (sWiredCallbackHandle == nullptr) {
    status = WIREDSESTATUS_FAILED;
  } else {
    mWiredSeHandle = sWiredCallbackHandle->openWiredSe();
    if (mWiredSeHandle <= 0) {
      status = WIREDSESTATUS_FAILED;
    } else {
      /* Do Nothing */
    }
  }
  return status;
}

Return<::android::hardware::secure_element::V1_0::SecureElementStatus>
WiredSe::seHalDeInit() {
  SecureElementStatus sestatus = SecureElementStatus::FAILED;
  int32_t wiredSeStatus = -1;

  if (sWiredCallbackHandle == nullptr) {
    sestatus = SecureElementStatus::FAILED;
  } else {
    wiredSeStatus = sWiredCallbackHandle->closeWiredSe(mWiredSeHandle);
  }
  if (wiredSeStatus < 0) {
    sestatus = SecureElementStatus::FAILED;
  } else {
    resetWiredSeContext();
  }
  return sestatus;
}

void WiredSe::resetWiredSeContext() {
  ALOGD("%s: Enter", __func__);
  mWiredSeHandle = 0;
  for (uint8_t xx = 0; xx < MAX_LOGICAL_CHANNELS; xx++) {
    mOpenedChannels[xx] = false;
  }
  mOpenedchannelCount = 0;
  return;
}

Return<void> WiredSe::setWiredSeCallback(
    const android::sp<INxpWiredSeHalCallback>& wiredCallback) {
  ALOGD("%s: Enter", __func__);
  /* Callback handle from NfcService WiredSe is copied and cached */
  sWiredCallbackHandle = wiredCallback;
  if (sWiredCallbackHandle == nullptr) {
    ALOGD("%s WiredSeCallback handle is NULL", __func__);
    if (pWiredSe != nullptr) pWiredSe->resetWiredSeContext();
    if (gSeHalCallback != nullptr) gSeHalCallback->onStateChange(false);
  } else if (gSeHalCallback != nullptr)
    gSeHalCallback->onStateChange(true);
  return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wired_se
}  // namespace nxp
}  // namespace vendor
