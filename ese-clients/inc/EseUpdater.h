/*******************************************************************************
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
#include <android/hardware/secure_element/1.0/ISecureElementHalCallback.h>
#include "phNxpEse_Api.h"
#include "eSEClientIntf.h"
#include "JcDnld.h"

#define eseUpdater (EseUpdater::getInstance())
using ::android::hardware::secure_element::V1_0::ISecureElementHalCallback;
extern eseUpdateInfo_t se_intf;
  typedef enum {
    ESE = 0,
    EUICC = 1,
  } SEDomainID;

class EseUpdater {
  public:
    static ese_update_state_t msEseUpdate;

/*****************************************************************************
**
** Function:        getInstance
**
** Description:     Get the EseUpdater singleton object.
**
** Returns:         EseUpdater object.
**
*******************************************************************************/
    static EseUpdater& getInstance();

/*******************************************************************************
**
** Function:        seteSEClientState
**
** Description:     Function to set the eSEUpdate state
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
    static void seteSEClientState(uint8_t state);

/*******************************************************************************
**
** Function:        handleJcopOsDownload
**
** Description:     Perform JCOP update
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
    static SESTATUS handleJcopOsDownload();

/*******************************************************************************
**
** Function:        eSEUpdate_SeqHandler
**
** Description:     ESE client update handler
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
    static SESTATUS eSEUpdate_SeqHandler();

/*******************************************************************************
**
** Function:        initializeEse
**
** Description:     Open & Initialize libese
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
    static SESTATUS initializeEse(phNxpEse_initMode mode,SEDomainID Id);

/*******************************************************************************
**
** Function:        checkIfEseClientUpdateReqd
**
** Description:     Check the initial condition
                    and interafce for eSE Client update for LS and JCOP download
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
    void checkIfEseClientUpdateReqd();

/*******************************************************************************
**
** Function:        sendeSEUpdateState
**
** Description:     Notify NFC HAL LS / JCOP download state
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
    static void sendeSEUpdateState(ese_update_state_t state);

/***************************************************************************
**
** Function:        doEseUpdateIfReqd
**
** Description:     Perform LS and JCOP download during hal service init
**
** Returns:         SUCCESS / SESTATUS_FAILED
**
*******************************************************************************/
    SESTATUS doEseUpdateIfReqd();

   private:
    EseUpdater();
    static EseUpdater sEseUpdaterInstance;
    static spSeChannel seChannelCallback;
    static spSeEvt seEventCallback;

/*******************************************************************************
**
** Function:        eSEClientUpdateHandler
**
** Description:     Perform JCOP Download
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
    void eSEClientUpdateHandler();
};

/*******************************************************************************
**
** Function:        eSEClientUpdate_SE_Thread
**
** Description:     Perform eSE update
**
** Returns:         None
**
*******************************************************************************/
void eSEClientUpdate_SE_Thread();
