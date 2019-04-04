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

#include "eSEClient.h"
#include <cutils/log.h>
#include <dirent.h>
#include <stdlib.h>
#include <IChannel.h>
#include <pthread.h>
#include <JcDnld.h>
#include "phNxpEse_Apdu_Api.h"
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <ese_config.h>
#include "phNxpEse_Spm.h"
#include <LsClient.h>
#include "hal_nxpese.h"
#include "NxpEse.h"
#include "NfcAdaptation.h"
#include <vendor/nxp/nxpnfc/1.0/INxpNfc.h>

using vendor::nxp::nxpese::V1_0::implementation::NxpEse;
using vendor::nxp::nxpnfc::V1_0::INxpNfc;
using android::sp;
using android::hardware::Void;
using android::hardware::hidl_vec;
sp<INxpNfc> mHalNxpNfc = nullptr;

void seteSEClientState(uint8_t state);

IChannel_t Ch;
se_extns_entry se_intf;
void* eSEClientUpdate_ThreadHandler(void* data);
void* eSEClientUpdate_Thread(void* data);
void* eSEUpdate_SE_SeqHandler(void* data);
void eSEClientUpdate_Thread();
SESTATUS ESE_ChannelInit(IChannel *ch);
SESTATUS handleJcopOsDownload();
void* LSUpdate_Thread(void* data);
uint8_t performLSUpdate();
SESTATUS initializeEse(phNxpEse_initMode mode, SEDomainID Id);
ese_update_state_t ese_update = ESE_UPDATE_COMPLETED;
SESTATUS eSEUpdate_SeqHandler();
int16_t SE_Open()
{
    return SESTATUS_SUCCESS;
}

void SE_Reset()
{
    //SESTATUS_SUCCESS;
}
bool SE_Transmit(uint8_t* xmitBuffer, int32_t xmitBufferSize, uint8_t* recvBuffer,
                     int32_t recvBufferMaxSize, int32_t& recvBufferActualSize, int32_t timeoutMillisec)
{
    phNxpEse_data cmdData;
    phNxpEse_data rspData;

    cmdData.len = xmitBufferSize;
    cmdData.p_data = xmitBuffer;

    recvBufferMaxSize++;
    timeoutMillisec++;

    if (ESESTATUS_SUCCESS != phNxpEse_Transceive(&cmdData, &rspData)) {
      ALOGE("%s: phNxpEse_Transceive Failed", __FUNCTION__);
    }

    recvBufferActualSize = rspData.len;

    if (rspData.p_data != NULL && rspData.len)
    {
      memcpy(&recvBuffer[0], rspData.p_data, rspData.len);
    }
        //free (rspData.p_data);
    //&recvBuffer = rspData.p_data;
    ALOGE("%s: size = 0x%x recv[0] = 0x%x", __FUNCTION__, recvBufferActualSize, recvBuffer[0]);
    return true;
}

void SE_JcopDownLoadReset()
{
    phNxpEse_resetJcopUpdate();
}

bool SE_Close(int16_t mHandle)
{
    if(mHandle != 0)
      return true;
    else
      return false;
}
uint8_t SE_getInterfaceInfo()
{
  return INTF_SE;
}

/***************************************************************************
**
** Function:        checkEseClientUpdate
**
** Description:     Check the initial condition
                    and interafce for eSE Client update for LS and JCOP download
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void checkEseClientUpdate()
{
  ALOGD("%s enter:  ", __func__);
  checkeSEClientRequired(ESE_INTF_SPI);
  se_intf.isJcopUpdateRequired = getJcopUpdateRequired();
  se_intf.isLSUpdateRequired = getLsUpdateRequired();
  se_intf.sJcopUpdateIntferface = getJcopUpdateIntf();
  se_intf.sLsUpdateIntferface = getLsUpdateIntf();
  if((se_intf.isJcopUpdateRequired && se_intf.sJcopUpdateIntferface)||
   (se_intf.isLSUpdateRequired && se_intf.sLsUpdateIntferface))
    seteSEClientState(ESE_UPDATE_STARTED);
}

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
  ALOGD("%s Ioctl Type=%lu",
         func,(unsigned long)pOutData->ioctlType);
  NfcAdaptation* pAdaptation = (NfcAdaptation*)pOutData->context;
  /*Output Data from stub->Proxy is copied back to output data
   * This data will be sent back to libnfc*/
  memcpy(&pAdaptation->mCurrentIoctlData->out, &outputData[0],
         sizeof(ese_nxp_ExtnOutputData_t));
  ALOGD("%s Ioctl Type status = %d ",
         func,pOutData->data.status);
}

/***************************************************************************
**
** Function:        perform_eSEClientUpdate
**
** Description:     Perform LS and JCOP download during hal service init
**
** Returns:         SUCCESS / SESTATUS_FAILED
**
*******************************************************************************/
SESTATUS perform_eSEClientUpdate() {
  ALOGD("%s enter:  ", __func__);

  eSEClientUpdate_Thread();
  return SESTATUS_SUCCESS;
}

SESTATUS ESE_ChannelInit(IChannel *ch)
{
    ch->open = SE_Open;
    ch->close = SE_Close;
    ch->transceive = SE_Transmit;
    ch->doeSE_Reset = SE_Reset;
    ch->doeSE_JcopDownLoadReset = SE_JcopDownLoadReset;
    ch->getInterfaceInfo = SE_getInterfaceInfo;
    return SESTATUS_SUCCESS;
}

/*******************************************************************************
**
** Function:        eSEClientUpdate_Thread
**
** Description:     Perform eSE update
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void eSEClientUpdate_Thread()
{
  SESTATUS status = SESTATUS_FAILED;
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (pthread_create(&thread, &attr, &eSEClientUpdate_ThreadHandler, NULL) != 0) {
    ALOGD("Thread creation failed");
    status = SESTATUS_FAILED;
  } else {
    status = SESTATUS_SUCCESS;
    ALOGD("Thread creation success");
  }
    pthread_attr_destroy(&attr);
}
/*******************************************************************************
**
** Function:        eSEClientUpdate_Thread
**
** Description:     Perform eSE update
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void eSEClientUpdate_SE_Thread()
{
  SESTATUS status = SESTATUS_FAILED;
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (pthread_create(&thread, &attr, &eSEUpdate_SE_SeqHandler, NULL) != 0) {
    ALOGD("Thread creation failed");
    status = SESTATUS_FAILED;
  } else {
    status = SESTATUS_SUCCESS;
    ALOGD("Thread creation success");
  }
    pthread_attr_destroy(&attr);
}
/*******************************************************************************
**
** Function:        eSEClientUpdate_ThreadHandler
**
** Description:     Perform JCOP Download
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void* eSEUpdate_SE_SeqHandler(void* data) {
  (void)data;
  ALOGD("%s Enter\n", __func__);
  eSEUpdate_SeqHandler();
  ALOGD("%s Exit eSEUpdate_SE_SeqHandler\n", __func__);
  return NULL;
}
/*******************************************************************************
**
** Function:        eSEClientUpdate_ThreadHandler
**
** Description:     Perform JCOP Download
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void* eSEClientUpdate_ThreadHandler(void* data) {
  (void)data;
  ese_nxp_IoctlInOutData_t inpOutData;
  int state, cnt = 0;

  ALOGD("%s Enter\n", __func__);
  while(((mHalNxpNfc == nullptr) && (cnt < 3)))
  {
    mHalNxpNfc = INxpNfc::tryGetService();
    if(mHalNxpNfc == nullptr)
      ALOGD(": Failed to retrieve the NXP NFC HAL!");
    if(mHalNxpNfc != nullptr) {
	ALOGD("INxpNfc::getService() returned %p (%s)",
		  mHalNxpNfc.get(),
               (mHalNxpNfc->isRemote() ? "remote" : "local"));
    }
    usleep(100*1000);
    cnt++;
  }

  if(mHalNxpNfc != nullptr)
  {
    memset(&inpOutData, 0x00, sizeof(ese_nxp_IoctlInOutData_t));
    inpOutData.inp.data.nxpCmd.cmd_len = sizeof(state);
    memcpy(inpOutData.inp.data.nxpCmd.p_cmd, &state,sizeof(state));

    hidl_vec<uint8_t> data;

    ese_nxp_IoctlInOutData_t* pInpOutData = &inpOutData;
    //ALOGD_IF(nfc_debug_enabled, "%s arg=%ld", func, arg);
    pInpOutData->inp.context = &NfcAdaptation::GetInstance();
    NfcAdaptation::GetInstance().mCurrentIoctlData = pInpOutData;
    data.setToExternal((uint8_t*)pInpOutData, sizeof(nfc_nci_IoctlInOutData_t));

    mHalNxpNfc->ioctl(HAL_NFC_IOCTL_GET_ESE_UPDATE_STATE, data, IoctlCallback);

    ALOGD("Ioctl Completed for Type result = %d", pInpOutData->out.data.status);
    if(!se_intf.isJcopUpdateRequired && (pInpOutData->out.data.status & 0xFF))
    {
	  se_intf.isJcopUpdateRequired = true;
    }
    if(!se_intf.isLSUpdateRequired && ((pInpOutData->out.data.status >> 8) & 0xFF))
    {
	  se_intf.isLSUpdateRequired = true;
    }
  }


  if(se_intf.isJcopUpdateRequired) {
    if(se_intf.sJcopUpdateIntferface == ESE_INTF_NFC) {
      seteSEClientState(ESE_JCOP_UPDATE_REQUIRED);
      return NULL;
    }
    else if(se_intf.sJcopUpdateIntferface == ESE_INTF_SPI) {
      seteSEClientState(ESE_JCOP_UPDATE_REQUIRED);
    }
  }

  if((ESE_JCOP_UPDATE_REQUIRED != ese_update) && (se_intf.isLSUpdateRequired)) {
    if(se_intf.sLsUpdateIntferface == ESE_INTF_NFC) {
      seteSEClientState(ESE_LS_UPDATE_REQUIRED);
      return NULL;
    }
    else if(se_intf.sLsUpdateIntferface == ESE_INTF_SPI) {
      seteSEClientState(ESE_LS_UPDATE_REQUIRED);
    }
  }

  if((ese_update == ESE_JCOP_UPDATE_REQUIRED) ||
    (ese_update == ESE_LS_UPDATE_REQUIRED))
      eSEUpdate_SeqHandler();

  ALOGD("%s Exit eSEClientUpdate_Thread\n", __func__);
  return NULL;
}

/*******************************************************************************
**
** Function:        handleJcopOsDownload
**
** Description:     Perform JCOP update
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
SESTATUS handleJcopOsDownload()
{
  SESTATUS status = SESTATUS_FAILED;
  uint8_t retstat;
  status = initializeEse(ESE_MODE_OSU, ESE);
  if(status == SESTATUS_SUCCESS)
  {
    retstat = JCDNLD_Init(&Ch);
    if(retstat != STATUS_SUCCESS)
    {
      ALOGE("%s: JCDND initialization failed", __FUNCTION__);
      phNxpEse_ResetEndPoint_Cntxt(0);
      phNxpEse_close();
      return status;
    } else
    {
      retstat = JCDNLD_StartDownload();
      if(retstat != SESTATUS_SUCCESS)
      {
        ALOGE("%s: JCDNLD_StartDownload failed", __FUNCTION__);
      }
    }
    JCDNLD_DeInit();
    phNxpEse_ResetEndPoint_Cntxt(0);
    phNxpEse_close();
  }
  status = SESTATUS_SUCCESS;
  return status;
}

/*******************************************************************************
**
** Function:        performLSUpdate
**
** Description:     Perform LS update
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
uint8_t performLSUpdate()
{
  const char* SEterminal = "eSEx";
  bool ret = false;
  char terminalID[5];
  uint8_t status = SESTATUS_FAILED;
  bool isSEPresent = false;
  bool isVISOPresent = false;
  ret = geteSETerminalId(terminalID);
  ALOGI("performLSUpdate Terminal val = %s", terminalID);
  if((ret) && (strncmp(SEterminal, terminalID, 3) == 0))
  {
    isSEPresent = true;
  }
  ret = geteUICCTerminalId(terminalID);
  if((ret) && (strncmp(SEterminal, terminalID, 3) == 0))
  {
    isVISOPresent = true;
  }
  seteSEClientState(ESE_UPDATE_STARTED);
  if(isSEPresent)
  {
    ALOGE("%s:On eSE domain ", __FUNCTION__);
    status = initializeEse(ESE_MODE_NORMAL, ESE);
    ALOGE("%s:On eSE domain ", __FUNCTION__);
    if(status == SESTATUS_SUCCESS)
    {
      status = performLSDownload(&Ch);
      phNxpEse_ResetEndPoint_Cntxt(ESE);
    }
    phNxpEse_close();
  }
  if(isVISOPresent)
  {
    ALOGE("%s:On eUICC domain ", __FUNCTION__);
    status = initializeEse(ESE_MODE_NORMAL, EUICC);
    if(status == SESTATUS_SUCCESS)
    {
      status = performLSDownload(&Ch);
      phNxpEse_ResetEndPoint_Cntxt(EUICC);
    }
    phNxpEse_close();
  }
  return status;
}

/*******************************************************************************
**
** Function:        initializeEse
**
** Description:     Open & Initialize libese
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
SESTATUS initializeEse(phNxpEse_initMode mode, SEDomainID Id)
{
  uint8_t retstat;
  SESTATUS status = SESTATUS_FAILED;
  phNxpEse_initParams initParams;
  memset(&initParams, 0x00, sizeof(phNxpEse_initParams));

  initParams.initMode = mode;
  ALOGE("%s: Mode = %d", __FUNCTION__, mode);
  retstat = phNxpEse_open(initParams);
  if (retstat != ESESTATUS_SUCCESS) {
    return status;
  }
  retstat = phNxpEse_SetEndPoint_Cntxt(Id);
  if (retstat != ESESTATUS_SUCCESS) {
    ALOGE("%s:phNxpEse_SetEndPoint_Cntxt failed!!!", __FUNCTION__);
  }
  retstat = phNxpEse_init(initParams);
  if(retstat != ESESTATUS_SUCCESS)
  {
    phNxpEse_ResetEndPoint_Cntxt(Id);
    phNxpEse_close();
    return status;
  }
  ESE_ChannelInit(&Ch);
  status = SESTATUS_SUCCESS;
  return status;
}

/*******************************************************************************
**
** Function:        seteSEClientState
**
** Description:     Function to set the eSEUpdate state
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void seteSEClientState(uint8_t state)
{
  ALOGE("%s: State = %d", __FUNCTION__, state);
  ese_update = (ese_update_state_t)state;
}

/*******************************************************************************
**
** Function:        sendeSEUpdateState
**
** Description:     Notify NFC HAL LS / JCOP download state
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void sendeSEUpdateState(uint8_t state)
{
  ALOGE("%s: State = %d", __FUNCTION__, state);
  phNxpEse_SPM_SetEseClientUpdateState(state);
}

/*******************************************************************************
**
** Function:        eSEUpdate_SeqHandler
**
** Description:     ESE client update handler
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
SESTATUS eSEUpdate_SeqHandler()
{
    switch(ese_update)
    {
      case ESE_UPDATE_STARTED:
        break;
      case ESE_JCOP_UPDATE_REQUIRED:
        ALOGE("%s: ESE_JCOP_UPDATE_REQUIRED", __FUNCTION__);
        if(se_intf.isJcopUpdateRequired) {
          if(se_intf.sJcopUpdateIntferface == ESE_INTF_SPI) {
            handleJcopOsDownload();
            sendeSEUpdateState(ESE_JCOP_UPDATE_COMPLETED);
          }
          else if(se_intf.sJcopUpdateIntferface == ESE_INTF_NFC) {
            return SESTATUS_SUCCESS;
          }
        }
        [[fallthrough]];
      case ESE_JCOP_UPDATE_COMPLETED:
        ALOGE("%s: ESE_JCOP_UPDATE_COMPLETED", __FUNCTION__);
        [[fallthrough]];
      case ESE_LS_UPDATE_REQUIRED:
        if(se_intf.isLSUpdateRequired) {
          if(se_intf.sLsUpdateIntferface == ESE_INTF_SPI) {
            performLSUpdate();
            sendeSEUpdateState(ESE_LS_UPDATE_COMPLETED);
          }
          else if(se_intf.sLsUpdateIntferface == ESE_INTF_NFC)
          {
            seteSEClientState(ESE_LS_UPDATE_REQUIRED);
            return SESTATUS_SUCCESS;
          }
        }
        ALOGE("%s: ESE_LS_UPDATE_REQUIRED", __FUNCTION__);
        [[fallthrough]];
      case ESE_LS_UPDATE_COMPLETED:
        ALOGE("%s: ESE_LS_UPDATE_COMPLETED", __FUNCTION__);
        [[fallthrough]];
      case ESE_UPDATE_COMPLETED:
        seteSEClientState(ESE_UPDATE_COMPLETED);
        sendeSEUpdateState(ESE_UPDATE_COMPLETED);
        NxpEse::initSEService();
        NxpEse::initVIrtualISOService();
        ALOGE("%s: ESE_UPDATE_COMPLETED", __FUNCTION__);
      break;
    }
    return SESTATUS_SUCCESS;
}
