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

#include "eSEClient.h"
#include <cutils/log.h>
#include <dirent.h>
#include <stdlib.h>
#include <pthread.h>
#include <IChannel.h>
#include <JcDnld.h>
#include "phNxpEse_Apdu_Api.h"
#include "phNxpEse_Api.h"
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>


IChannel_t Ch;
static const char *path[3] = {"/data/vendor/nfc/JcopOs_Update1.apdu",
                             "/data/vendor/nfc/JcopOs_Update2.apdu",
                             "/data/vendor/nfc/JcopOs_Update3.apdu"};

static const char *uai_path[2] = {"/data/vendor/nfc/cci.jcsh",
                                  "/data/vendor/nfc/jci.jcsh"};

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
    phNxpEse_Transceive(&cmdData, &rspData);

    recvBufferActualSize = rspData.len;
    memcpy(&recvBuffer[0], rspData.p_data, rspData.len);
    if (rspData.p_data != NULL)
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

SESTATUS ESE_ChannelInit(IChannel *ch)
{
    ch->open = SE_Open;
    ch->close = SE_Close;
    ch->transceive = SE_Transmit;
    ch->doeSE_Reset = SE_Reset;
    ch->doeSE_JcopDownLoadReset = SE_JcopDownLoadReset;
    return SESTATUS_SUCCESS;
}

/*******************************************************************************
**
** Function:        LSC_doDownload
**
** Description:     Perform LS during hal init
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
SESTATUS JCOS_doDownload(
    /*const android::sp<ISecureElementHalCallback>& clientCallback*/) {
  SESTATUS status = SESTATUS_FAILED;

  uint8_t retstat;
  phNxpEse_initParams initParams;

  ALOGD("%s enter:  ", __func__);

  bool stats = true;
  struct stat st;
  for (int num = 0; num < 2; num++)
  {
      if (stat(uai_path[num], &st))
      {
          stats = false;
      }
  }
  /*If UAI specific files are present*/
  if(stats == true)
  {
      for (int num = 0; num < 3; num++)
      {
          if (stat(path[num], &st))
          {
              stats = false;
          }
      }
  }
  if(stats)
  {
      ALOGD("%s Update required: Files present\n", __func__);
      memset(&initParams, 0x00, sizeof(phNxpEse_initParams));
      initParams.initMode = ESE_MODE_OSU;
      retstat = phNxpEse_open(initParams);
      if (retstat != ESESTATUS_SUCCESS) {
          return status;
      }
      phNxpEse_SetEndPoint_Cntxt(0);
      retstat = phNxpEse_init(initParams);
      if(retstat != ESESTATUS_SUCCESS)
      {
          phNxpEse_ResetEndPoint_Cntxt(0);
          phNxpEse_close();
          return status;
      }

      ESE_ChannelInit(&Ch);
      retstat = JCDNLD_Init(&Ch);
      if(retstat != STATUS_SUCCESS)
      {
          ALOGE("%s: JCDND initialization failed", __FUNCTION__);
          phNxpEse_ResetEndPoint_Cntxt(0);
          phNxpEse_close();
          return status;
      }else
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
      status = SESTATUS_SUCCESS;
  }
  else
  {
      ALOGD("%s Update not required: Files not present\n", __func__);
  }
  ALOGD("%s Exit JCOS_doDownload\n", __func__);
  return status;
}
