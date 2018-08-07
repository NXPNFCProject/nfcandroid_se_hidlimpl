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

/*
 * DAL spi port implementation for linux
 *
 * Project: Trusted ESE Linux
 *
 */
#define LOG_TAG "NxpEseHal"
#include <log/log.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "NfcAdaptation.h"
#include "StateMachine.h"
#include "StateMachineInfo.h"
#include "hal_nxpese.h"
#include "phNxpEse_Api.h"
#include <ese_config.h>
#include <hardware/nfc.h>
#include <phEseStatus.h>
#include <phNxpEsePal.h>
#include <phNxpEsePal_spi.h>
#include <string.h>

#define MAX_RETRY_CNT 10
#define HAL_NFC_SPI_DWP_SYNC 21
#define RF_ON 1

extern int omapi_status;
extern bool ese_debug_enabled;
extern SyncEvent gSpiTxLock;
extern SyncEvent gSpiOpenLock;

static int rf_status;
unsigned long int configNum1, configNum2;

static const uint8_t MAX_SPI_WRITE_RETRY_COUNT_HW_ERR = 3;
static IntervalTimer sTimerInstance;
/*******************************************************************************
**
** Function         phPalEse_spi_close
**
** Description      Closes PN547 device
**
** Parameters       pDevHandle - device handle
**
** Returns          None
**
*******************************************************************************/
void phPalEse_spi_close(void* pDevHandle) {
  if (NULL != pDevHandle) {
    close((intptr_t)pDevHandle);
  }
  ALOGD_IF(ese_debug_enabled, "halimpl close exit.");
  return;
}
/*******************************************************************************
**
** Function         phPalEse_spi_close
**
** Description      Closes PN547 device
**
** Parameters       pDevHandle - device handle
**
** Returns          None
**
*******************************************************************************/
void phPalEse_spi_dwp_sync_close() {
  ese_nxp_IoctlInOutData_t inpOutData;
  static uint8_t cmd_omapi_concurrent[] = {0x2F, 0x01, 0x01, 0x00};
  int retval;
  ALOGD_IF(ese_debug_enabled, "halimpl close enter.");

  NfcAdaptation& pNfcAdapt = NfcAdaptation::GetInstance();
  pNfcAdapt.Initialize();
  // nxpesehal_ctrl.p_ese_stack_cback = p_cback;
  // nxpesehal_ctrl.p_ese_stack_data_cback = p_data_cback;
  memset(&inpOutData, 0x00, sizeof(ese_nxp_IoctlInOutData_t));
  inpOutData.inp.data.nxpCmd.cmd_len = sizeof(cmd_omapi_concurrent);
  inpOutData.inp.data_source = 1;
  memcpy(inpOutData.inp.data.nxpCmd.p_cmd, cmd_omapi_concurrent,
         sizeof(cmd_omapi_concurrent));
  retval = pNfcAdapt.HalIoctl(HAL_NFC_SPI_DWP_SYNC, &inpOutData);
  ALOGD_IF(ese_debug_enabled, "_spi_close() status %x", retval);
}

ESESTATUS phNxpEse_spiIoctl(uint64_t ioctlType, void* p_data) {
  ese_nxp_IoctlInOutData_t *inpOutData;
  if (p_data != NULL) {
    inpOutData = (ese_nxp_IoctlInOutData_t *)p_data;
    ALOGD_IF(ese_debug_enabled, "phNxpEse_spiIoctl(): ioctlType: %ld",
             (long)ioctlType);
  }
  switch (ioctlType) {
  case HAL_NFC_IOCTL_RF_STATUS_UPDATE: {
    rf_status = inpOutData->inp.data.nxpCmd.p_cmd[0];
    if (rf_status == 1) {
      ALOGD_IF(
          ese_debug_enabled,
          "*******************RF IS ON*************************************");
      phPalEse_spi_stop_debounce_timer();
      StateMachine::GetInstance().ProcessExtEvent(EVT_RF_ON);
    } else {
      ALOGD_IF(
          ese_debug_enabled,
          "*******************RF IS OFF************************************");
      phPalEse_spi_start_debounce_timer(500);
    }
  } break;
  case HAL_NFC_IOCTL_RF_ACTION_NTF: {
    ALOGD_IF(
        ese_debug_enabled,
        "*******************RF ACT NTF*************************************");
    /* Parsing NFCEE Action Notification to detect type of routing either SCBR
     * or Technology F for ESE to resume SPI session for ESE-UICC concurrency */
    if ((inpOutData->inp.data.nxpCmd.p_cmd[0] == 0xC0) &&
        ((inpOutData->inp.data.nxpCmd.p_cmd[1] == 0x03) ||
         ((inpOutData->inp.data.nxpCmd.p_cmd[1] == 0x02) &&
          (inpOutData->inp.data.nxpCmd.p_cmd[3] == 0x02)))) {
      StateMachine::GetInstance().ProcessExtEvent(EVT_RF_ACT_NTF_ESE_F);
      {
        SyncEventGuard guard(gSpiTxLock);
        ALOGD_IF(ese_debug_enabled, "%s: Notifying SPI_TX Wait if waiting...",
                 __FUNCTION__);
        gSpiTxLock.notifyOne();
      }
    }
  } break;
  default:
    break;
  }
  return ESESTATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         phPalEse_spi_open_and_configure
**
** Description      Open and configure pn547 device
**
** Parameters       pConfig     - hardware information
**                  pLinkHandle - device handle
**
** Returns          ESE status:
**                  ESESTATUS_SUCCESS            - open_and_configure operation
*success
**                  ESESTATUS_INVALID_DEVICE     - device open operation failure
**
*******************************************************************************/
ESESTATUS phPalEse_spi_open_and_configure(pphPalEse_Config_t pConfig) {
  int nHandle;
  int retryCnt = 0, nfc_access_retryCnt = 0;
  int retval;
  ese_nxp_IoctlInOutData_t inpOutData;
  NfcAdaptation& pNfcAdapt = NfcAdaptation::GetInstance();
  pNfcAdapt.Initialize();
  static uint8_t cmd_omapi_concurrent[] = {0x2F, 0x01, 0x01, 0x01};

  if (EseConfig::hasKey(NAME_NXP_SOF_WRITE)) {
    configNum1 = EseConfig::getUnsigned(NAME_NXP_SOF_WRITE);
    ALOGD_IF(ese_debug_enabled, "NXP_SOF_WRITE value from config file = %ld",
             configNum1);
  }

  if (EseConfig::hasKey(NAME_NXP_SPI_WRITE_TIMEOUT)) {
    configNum2 = EseConfig::getUnsigned(NAME_NXP_SPI_WRITE_TIMEOUT);
    ALOGD_IF(ese_debug_enabled,
             "NXP_SPI_WRITE_TIMEOUT value from config file = %ld", configNum2);
  }
  ALOGD_IF(ese_debug_enabled, "halimpl open enter.");
  memset(&inpOutData, 0x00, sizeof(ese_nxp_IoctlInOutData_t));
  inpOutData.inp.data.nxpCmd.cmd_len = sizeof(cmd_omapi_concurrent);
  inpOutData.inp.data_source = 1;
  memcpy(inpOutData.inp.data.nxpCmd.p_cmd, cmd_omapi_concurrent,
         sizeof(cmd_omapi_concurrent));

retry_nfc_access:
  omapi_status = ESESTATUS_FAILED;
  retval = pNfcAdapt.HalIoctl(HAL_NFC_SPI_DWP_SYNC, &inpOutData);
  if (omapi_status != 0) {
    ALOGD_IF(ese_debug_enabled, "omapi_status return failed.");
    nfc_access_retryCnt++;
    phPalEse_sleep(2000000);
    if (nfc_access_retryCnt < 5) goto retry_nfc_access;
    return ESESTATUS_FAILED;
  }
  ALOGD_IF(ese_debug_enabled, "halimpl open exit");
  /* open port */
  ALOGD_IF(ese_debug_enabled, "Opening port=%s\n", pConfig->pDevName);
retry:
  nHandle = open((char const*)pConfig->pDevName, O_RDWR);
  if (nHandle < 0) {
    ALOGE("%s : failed errno = 0x%x", __FUNCTION__, errno);
    if (errno == -EBUSY || errno == EBUSY) {
      retryCnt++;
      ALOGE("Retry open eSE driver, retry cnt : %d", retryCnt);
      if (retryCnt < MAX_RETRY_CNT) {
        phPalEse_sleep(1000000);
        goto retry;
      }
    }
    ALOGE("_spi_open() Failed: retval %x", nHandle);
    pConfig->pDevHandle = NULL;
    return ESESTATUS_INVALID_DEVICE;
  }
  ALOGD_IF(ese_debug_enabled, "eSE driver opened :: fd = [%d]", nHandle);
  pConfig->pDevHandle = (void*)((intptr_t)nHandle);
  return ESESTATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         phPalEse_spi_read
**
** Description      Reads requested number of bytes from pn547 device into given
*buffer
**
** Parameters       pDevHandle       - valid device handle
**                  pBuffer          - buffer for read data
**                  nNbBytesToRead   - number of bytes requested to be read
**
** Returns          numRead   - number of successfully read bytes
**                  -1        - read operation failure
**
*******************************************************************************/
int phPalEse_spi_read(void* pDevHandle, uint8_t* pBuffer, int nNbBytesToRead) {
  int ret = -1;
  ALOGD_IF(ese_debug_enabled, "%s Read Requested %d bytes", __FUNCTION__,
           nNbBytesToRead);
  ret = read((intptr_t)pDevHandle, (void*)pBuffer, (nNbBytesToRead));
  ALOGD_IF(ese_debug_enabled, "Read Returned = %d", ret);
  return ret;
}

/*******************************************************************************
**
** Function         phPalEse_spi_write
**
** Description      Writes requested number of bytes from given buffer into
*pn547 device
**
** Parameters       pDevHandle       - valid device handle
**                  pBuffer          - buffer for read data
**                  nNbBytesToWrite  - number of bytes requested to be written
**
** Returns          numWrote   - number of successfully written bytes
**                  -1         - write operation failure
**
*******************************************************************************/
int phPalEse_spi_write(void* pDevHandle, uint8_t* pBuffer,
                       int nNbBytesToWrite) {
  int ret = -1;
  int numWrote = 0;
  unsigned long int retryCount = 0;

  if (NULL == pDevHandle) {
    return -1;
  }

  if (configNum1 == 1) {
    /* Appending SOF for SPI write */
    pBuffer[0] = SEND_PACKET_SOF;
  } else {
    /* Do Nothing */
  }

  while (numWrote < nNbBytesToWrite) {
    // usleep(5000);
    ret = write((intptr_t)pDevHandle, pBuffer + numWrote,
                nNbBytesToWrite - numWrote);
    if (ret > 0) {
      numWrote += ret;
    } else if (ret == 0) {
      ALOGE("_spi_write() EOF");
      return -1;
    } else {
      ALOGE("_spi_write() failed errno : %x", errno);
      if (retryCount < MAX_SPI_WRITE_RETRY_COUNT_HW_ERR) {
        retryCount++;
        /*wait for eSE wake up*/
        phPalEse_sleep(WRITE_WAKE_UP_DELAY);
        ALOGE("_spi_write() failed. Going to retry, counter:%ld !", retryCount);
        continue;
      }
      return -1;
    }
  }
  return numWrote;
}

/*******************************************************************************
**
** Function         phPalEse_spi_ioctl
**
** Description      Exposed ioctl by p61 spi driver
**
** Parameters       pDevHandle     - valid device handle
**                  level          - reset level
**
** Returns           0   - ioctl operation success
**                  -1   - ioctl operation failure
**
*******************************************************************************/
ESESTATUS phPalEse_spi_ioctl(phPalEse_ControlCode_t eControlCode,
                             void* pDevHandle, long level) {
  ESESTATUS ret = ESESTATUS_IOCTL_FAILED;
  ALOGD_IF(ese_debug_enabled, "phPalEse_spi_ioctl(), ioctl %x , level %lx",
           eControlCode, level);
  ese_nxp_IoctlInOutData_t inpOutData;
  inpOutData.inp.level = level;
  NfcAdaptation& pNfcAdapt = NfcAdaptation::GetInstance();
  if (NULL == pDevHandle) {
    return ESESTATUS_IOCTL_FAILED;
  }
  switch (eControlCode) {
    // Nfc Driver communication part
    case phPalEse_e_ChipRst:
      ret = pNfcAdapt.HalIoctl(HAL_NFC_SET_SPM_PWR, &inpOutData);
      break;

    case phPalEse_e_SetPowerScheme:
      // ret = sendIoctlData(p, HAL_NFC_SET_POWER_SCHEME, &inpOutData);
      ret = ESESTATUS_SUCCESS;
      break;

    case phPalEse_e_GetSPMStatus:
      // ret = sendIoctlData(p, HAL_NFC_GET_SPM_STATUS, &inpOutData);
      ret = ESESTATUS_SUCCESS;
      break;

    case phPalEse_e_GetEseAccess:
      // ret = sendIoctlData(p, HAL_NFC_GET_ESE_ACCESS, &inpOutData);
      ret = ESESTATUS_SUCCESS;
      break;
#ifdef NXP_ESE_JCOP_DWNLD_PROTECTION
    case phPalEse_e_SetJcopDwnldState:
      // ret = sendIoctlData(p, HAL_NFC_SET_DWNLD_STATUS, &inpOutData);
      ret = ESESTATUS_SUCCESS;
      break;
#endif
    default:
      ret = ESESTATUS_IOCTL_FAILED;
      break;
  }
  return ret;
}

/*******************************************************************************
**
** Function         phPalEse_spi_rf_off_timer_expired_cb
**
** Description      Sends event RF-OFF after expiry of debounce timer.
**
** Parameters       union sigval
**
** Returns          none
**
*******************************************************************************/
void phPalEse_spi_rf_off_timer_expired_cb(union sigval) {
  ALOGD_IF(true, "RF debounce timer expired...");
  StateMachine::GetInstance().ProcessExtEvent(EVT_RF_OFF);
  // just to be sure that we acquired dwp channel before allowing any activity
  // on SPI
  usleep(100);
  {
    SyncEventGuard guard(gSpiTxLock);
    ALOGD_IF(ese_debug_enabled, "%s: Notifying SPI_TX Wait if waiting...",
             __FUNCTION__);
    gSpiTxLock.notifyOne();
  }
  {
    SyncEventGuard guard(gSpiOpenLock);
    ALOGD_IF(ese_debug_enabled, "%s: Notifying SPI_OPEN Wait if waiting...",
             __FUNCTION__);
    gSpiOpenLock.notifyOne();
  }
}

/*******************************************************************************
**
** Function         phPalEse_spi_start_debounce_timer
**
** Description      Starts a rf debounce timer
**
** Parameters       unsigned long millisecs
**
** Returns          none
**
*******************************************************************************/
void phPalEse_spi_start_debounce_timer(unsigned long millisecs) {
  if (sTimerInstance.set(millisecs, phPalEse_spi_rf_off_timer_expired_cb) ==
      true) {
    ALOGD_IF(true, "Starting RF debounce timer...");
  } else {
    ALOGE_IF(true, "Error, Starting timer...");
  }
}

/*******************************************************************************
**
** Function         phPalEse_spi_stop_debounce_timer
**
** Description      Stops the rf debounce timer.
**
** Parameters       none
**
** Returns          none
**
*******************************************************************************/
void phPalEse_spi_stop_debounce_timer() {
  ALOGD_IF(true, "Stopping RF debounce timer...");
  sTimerInstance.kill();
}
