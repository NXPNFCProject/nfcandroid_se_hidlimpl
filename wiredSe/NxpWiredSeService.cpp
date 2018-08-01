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
#define LOG_TAG "wired_se@1.0-service"
#include <android/hardware/secure_element/1.0/ISecureElement.h>
#include <hidl/LegacySupport.h>
#include <log/log.h>
#include <vendor/nxp/nxpwiredse/1.0/INxpWiredSe.h>
#include "NxpWiredSe.h"
#include "WiredSe.h"
#include "eSEClientIntf.h"

using android::hardware::secure_element::V1_0::ISecureElement;
using vendor::nxp::wired_se::V1_0::implementation::WiredSe;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::OK;
using android::sp;
using android::status_t;
using vendor::nxp::nxpwiredse::V1_0::INxpWiredSe;
using vendor::nxp::nxpwiredse::V1_0::implementation::NxpWiredSe;
sp<WiredSe> pWiredSe;

int main() {
  char terminalID[5];
  status_t status;
  const char* NfcSeterminal = "eSEx";

  ALOGD("WiredSe HAL Service is starting.");
  pWiredSe = new WiredSe();
  sp<ISecureElement> ese_wired_service(pWiredSe.get());
  configureRpcThreadpool(1, true /*callerWillJoin*/);

  bool ret = getNfcSeTerminalId(terminalID);
  if((ret) && (strncmp(NfcSeterminal, terminalID, 3) == 0))
  {
    ALOGI("Terminal ID found");
    status = ese_wired_service->registerAsService(terminalID);
    if (status != OK) {
      LOG_ALWAYS_FATAL("Could not register service for Ese Wired HAL Iface (%d).",
                       status);
      return -1;
    }
    /* TODO check for valid name and log message*/
    sp<INxpWiredSe> nxp_ese_wired_service = new NxpWiredSe();
    status = nxp_ese_wired_service->registerAsService();
    if (status != OK) {
      LOG_ALWAYS_FATAL(
          "Could not register service for Nxp Wired Secure Element Extn Iface "
          "(%d).",
          status);
      return -1;
    }
  }

  ALOGD("WiredSe HAL Service is ready");
  joinRpcThreadpool();
  return 1;
}
