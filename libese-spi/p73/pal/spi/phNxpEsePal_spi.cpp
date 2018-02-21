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

#include <android-base/stringprintf.h>
#include <base/logging.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <hardware/nfc.h>
#include <phEseStatus.h>
#include <ese_config.h>
#include <phNxpEsePal.h>
#include <phNxpEsePal_spi.h>
#include <string.h>
#include "hal_nxpese.h"
#include "hal_nxpnfc.h"
#include "phNxpEse_Api.h"
#include "NfcAdaptation.h"

using android::base::StringPrintf;

#define MAX_RETRY_CNT 10
#define HAL_NFC_SPI_DWP_SYNC 21
extern int omapi_status;
static int rf_status;
unsigned long int configNum1, configNum2;
// Default max retry count for SPI CLT write blocked in secs
unsigned long int MAX_SPI_WRITE_RETRY_COUNT = 10;
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
  ese_nxp_IoctlInOutData_t inpOutData;
  static uint8_t cmd_omapi_concurrent[] = {0x2F, 0x01, 0x01, 0x00};
  DLOG_IF(INFO, ese_debug_enabled)
      << StringPrintf("halimpl close enter................");
  // nxpesehal_ctrl.p_ese_stack_cback = p_cback;
  // nxpesehal_ctrl.p_ese_stack_data_cback = p_data_cback;
  memset(&inpOutData, 0x00, sizeof(ese_nxp_IoctlInOutData_t));
  inpOutData.inp.data.nxpCmd.cmd_len = sizeof(cmd_omapi_concurrent);
  inpOutData.inp.data_source = 1;
  memcpy(inpOutData.inp.data.nxpCmd.p_cmd, cmd_omapi_concurrent,
         sizeof(cmd_omapi_concurrent));
  // retval = sendIoctlData(p, HAL_NFC_SPI_DWP_SYNC, &inpOutData);
  DLOG_IF(INFO, ese_debug_enabled)
      << StringPrintf("halimpl close exit................");

  if (NULL != pDevHandle) {
    close((intptr_t)pDevHandle);
  }

  return;
}
ESESTATUS phNxpEse_spiIoctl(uint64_t ioctlType, void* p_data) {
  nfc_nci_IoctlInOutData_t* inpOutData = (nfc_nci_IoctlInOutData_t*)p_data;
  rf_status = inpOutData->inp.data.nciCmd.p_cmd[0];
  if (rf_status == 1){
    DLOG_IF(INFO, ese_debug_enabled)
      << StringPrintf("******************RF IS ON*************************************");
  }
  else{
    DLOG_IF(INFO, ese_debug_enabled)
      << StringPrintf("******************RF IS OFF*************************************");
  }
  if (p_data != NULL){
    DLOG_IF(INFO, ese_debug_enabled)
      << StringPrintf("halimpl phNxpEse_spiIoctl p_data is not null ioctltyp: %ld",(long)ioctlType);
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
  ese_nxp_IoctlInOutData_t inpOutData;
  //NfcAdaptation& pNfcAdapt = NfcAdaptation::GetInstance();
  //pNfcAdapt.Initialize();
  static uint8_t cmd_omapi_concurrent[] = {0x2F, 0x01, 0x01, 0x01};

#ifdef ESE_DEBUG_UTILS_INCLUDED
  if (EseConfig::hasKey(NAME_NXP_SOF_WRITE)) {
    configNum1 = EseConfig::getUnsigned(NAME_NXP_SOF_WRITE);
    DLOG_IF(INFO, ese_debug_enabled)
      << StringPrintf("NXP_SOF_WRITE value from config file = %ld", configNum1);
  }
#endif

  if (EseConfig::hasKey(NAME_NXP_SPI_WRITE_TIMEOUT)) {
    configNum2 = EseConfig::getUnsigned(NAME_NXP_SPI_WRITE_TIMEOUT);
    DLOG_IF(INFO, ese_debug_enabled)
      << StringPrintf("NXP_SPI_WRITE_TIMEOUT value from config file = %ld", configNum2);
  }
  DLOG_IF(INFO, ese_debug_enabled)
      << StringPrintf("halimpl open enter................");
  memset(&inpOutData, 0x00, sizeof(ese_nxp_IoctlInOutData_t));
  inpOutData.inp.data.nxpCmd.cmd_len = sizeof(cmd_omapi_concurrent);
  inpOutData.inp.data_source = 1;
  memcpy(inpOutData.inp.data.nxpCmd.p_cmd, cmd_omapi_concurrent,
         sizeof(cmd_omapi_concurrent));

retry_nfc_access:
  omapi_status = ESESTATUS_FAILED;
  omapi_status = ESESTATUS_SUCCESS;
  //retval = pNfcAdapt.HalIoctl(HAL_NFC_SPI_DWP_SYNC, &inpOutData);
  if (omapi_status != 0) {
    DLOG_IF(INFO, ese_debug_enabled)
      << StringPrintf("omapi_status return failed................ ");
    nfc_access_retryCnt++;
    phPalEse_sleep(2000000);
    if (nfc_access_retryCnt < 5) goto retry_nfc_access;
    return ESESTATUS_FAILED;
  }

  DLOG_IF(INFO, ese_debug_enabled)
      << StringPrintf("Opening port=%s\n", pConfig->pDevName);
/* open port */

retry:
  nHandle = open((char const*)pConfig->pDevName, O_RDWR);
  if (nHandle < 0) {
    LOG(ERROR) << StringPrintf("%s : failed errno = 0x%x", __FUNCTION__, errno);
    if (errno == -EBUSY || errno == EBUSY) {
      retryCnt++;
      LOG(ERROR) << StringPrintf("Retry open eSE driver, retry cnt : %d", retryCnt);
      if (retryCnt < MAX_RETRY_CNT) {
        phPalEse_sleep(1000000);
        goto retry;
      }
    }
    LOG(ERROR) << StringPrintf("_spi_open() Failed: retval %x", nHandle);
    pConfig->pDevHandle = NULL;
    return ESESTATUS_INVALID_DEVICE;
  }
  DLOG_IF(INFO, ese_debug_enabled)
      << StringPrintf("eSE driver opened :: fd = [%d]", nHandle);
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
  DLOG_IF(INFO, ese_debug_enabled)
      << StringPrintf("%s Read Requested %d bytes", __FUNCTION__, nNbBytesToRead);
  ret = read((intptr_t)pDevHandle, (void*)pBuffer, (nNbBytesToRead));
  DLOG_IF(INFO, ese_debug_enabled)
      << StringPrintf("Read Returned = %d", ret);
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
#if 0
#ifdef ESE_DEBUG_UTILS_INCLUDED
  if (configNum1 == 1) {
    /* Appending SOF for SPI write */
    pBuffer[0] = SEND_PACKET_SOF;
  } else {
    /* Do Nothing */
  }
#else
  pBuffer[0] = SEND_PACKET_SOF;
#endif
#endif
  LOG(ERROR) << StringPrintf("NXP_SPI_WRITE_TIMEOUT value is... : %ld secs", configNum2);
  if (configNum2 > 0) {
    MAX_SPI_WRITE_RETRY_COUNT = configNum2;
    LOG(ERROR) << StringPrintf(" spi_write_timeout Wait time ... : %ld",
                 MAX_SPI_WRITE_RETRY_COUNT);
  } else {
    /* Do Nothing */
  }

  while (numWrote < nNbBytesToWrite) {
    // usleep(5000);
    if (rf_status == 0) {
      ret = write((intptr_t)pDevHandle, pBuffer + numWrote,
                  nNbBytesToWrite - numWrote);
    } else {
      ret = -1;
    }
    if (ret > 0) {
      numWrote += ret;
    } else if (ret == 0) {
      LOG(ERROR) << StringPrintf("_spi_write() EOF");
      return -1;
    } else {
      LOG(ERROR) << StringPrintf("_spi_write() errno : %x", errno);
      DLOG_IF(INFO, ese_debug_enabled)
      << StringPrintf("rf_status value is %d", rf_status);
      if ((errno == EINTR || errno == EAGAIN || rf_status == 1) &&
          (retryCount < MAX_SPI_WRITE_RETRY_COUNT)) {
        /*Configure retry count or timeout here,now its configured for 2*10
         * secs*/
        if (retryCount > MAX_SPI_WRITE_RETRY_COUNT) {
          ret = -1;
          break;
        }

        retryCount++;
        /* 5ms delay to give ESE wake up delay */
        phPalEse_sleep(1000 * WAKE_UP_DELAY);
        LOG(ERROR) << StringPrintf("_spi_write() failed. Going to retry, counter:%ld !",
                     retryCount);
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
ESESTATUS phPalEse_spi_ioctl(phPalEse_ControlCode_t eControlCode, void* pDevHandle,
                       long level) {
  ESESTATUS ret = ESESTATUS_IOCTL_FAILED;
  DLOG_IF(INFO, ese_debug_enabled)
      << StringPrintf("phPalEse_spi_ioctl(), ioctl %x , level %lx", eControlCode,
               level);
  nfc_nci_IoctlInOutData_t inpOutData;
  inpOutData.inp.level = level;
  //NfcAdaptation& pNfcAdapt = NfcAdaptation::GetInstance();
  if (NULL == pDevHandle) {
    return ESESTATUS_IOCTL_FAILED;
  }
  switch (eControlCode) {
    // Nfc Driver communication part
    case phPalEse_e_ChipRst:
      //ret = pNfcAdapt.HalIoctl(HAL_NFC_SET_SPM_PWR, &inpOutData);
      ret = ESESTATUS_SUCCESS;
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
#if (NXP_ESE_JCOP_DWNLD_PROTECTION == true)
    case phPalEse_e_SetJcopDwnldState:
      // ret = sendIoctlData(p, HAL_NFC_SET_DWNLD_STATUS, &inpOutData);
      ret = ESESTATUS_SUCCESS;
      break;
#endif
    case phPalEse_e_DisablePwrCntrl:
      // ret = sendIoctlData(p,HAL_NFC_INHIBIT_PWR_CNTRL,&inpOutData);
      ret = ESESTATUS_SUCCESS;
      break;
    default:
      ret = ESESTATUS_IOCTL_FAILED;
      break;
  }
  return ret;
}
