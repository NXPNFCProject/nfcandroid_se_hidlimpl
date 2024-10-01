/******************************************************************************
 *
 *  Copyright 2018-2019, 2022-2024 NXP
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
#define LOG_TAG "NxpEseHal"
#include "phNxpEse_Spm.h"

#include <errno.h>
#include <ese_logs.h>
#include <fcntl.h>
#include <log/log.h>
#include <phNxpEsePal.h>
#include <phNxpEse_Internal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "phNxpEseFeatures.h"

/*********************** Global Variables *************************************/

static void* pEseDeviceHandle = NULL;
#define MAX_ESE_ACCESS_TIME_OUT_MS 2000 /*2 seconds*/

/**
 * \addtogroup SPI_Power_Management
 *
 * @{ */
/******************************************************************************
\section Introduction Introduction

 * This module provide power request to Pn54x nfc-i2c driver, it checks if
 * wired access is already granted. It should have access to pn54x drive.
 * Below are the apis provided by the SPM module.
 ******************************************************************************/
/******************************************************************************
 * Function         phNxpEse_SPM_Init
 *
 * Description      This function opens the nfc i2c driver to manage power
 *                  and synchronization for ese secure element.
 *
 * Returns          On Success ESESTATUS_SUCCESS else proper error code
 *
 ******************************************************************************/
ESESTATUS phNxpEse_SPM_Init(void* pDevHandle) {
  ESESTATUS status = ESESTATUS_SUCCESS;
  pEseDeviceHandle = pDevHandle;
  if (NULL == pEseDeviceHandle) {
    NXP_LOG_ESE_E("%s : failed, device handle is null", __FUNCTION__);
    status = ESESTATUS_FAILED;
  }
  NXP_LOG_ESE_D("%s : exit status = %d", __FUNCTION__, status);

  return status;
}

/******************************************************************************
 * Function         phNxpEse_SPM_DeInit
 *
 * Description      This function closes the nfc i2c driver node.
 *
 * Returns          Always returns ESESTATUS_SUCCESS
 *
 ******************************************************************************/
ESESTATUS phNxpEse_SPM_DeInit(void) {
  pEseDeviceHandle = NULL;
  return ESESTATUS_SUCCESS;
}

/******************************************************************************
 * Function         phNxpEse_SPM_ConfigPwr
 *
 * Description      This function request to the nfc i2c driver
 *                  to enable/disable power to ese. This api should be called
 *before
 *                  sending any apdu to ese/once apdu exchange is done.
 *
 * Returns          On Success ESESTATUS_SUCCESS else proper error code
 *
 ******************************************************************************/
ESESTATUS phNxpEse_SPM_ConfigPwr(int arg) {
  int32_t ret = -1;
  ESESTATUS wSpmStatus = ESESTATUS_SUCCESS;
  /*None of the IOCTLs valid except SPM_RECOVERY_RESET*/
  if (arg != SPM_RECOVERY_RESET) {
    return ESESTATUS_SUCCESS;
  }
  ret = phPalEse_ioctl(phPalEse_e_ChipRst, pEseDeviceHandle, arg);
  if(ret < 0) {
    NXP_LOG_ESE_E("%s : failed errno = 0x%x", __FUNCTION__, errno);
    wSpmStatus = ESESTATUS_FAILED;
  }

  return wSpmStatus;
}

/******************************************************************************
 * Function         phNxpEse_SPM_SetEseClientUpdateState
 *
 * Description      This function is used to set the ese Update state
 *
 * Returns          On Success ESESTATUS_SUCCESS else proper error code
 *
 ******************************************************************************/
ESESTATUS phNxpEse_SPM_SetEseClientUpdateState(long arg) {
  int ret = -1;
  ESESTATUS status = ESESTATUS_SUCCESS;

  NXP_LOG_ESE_D("%s :phNxpEse_SPM_SetEseClientUpdateState  = 0x%ld",
                __FUNCTION__, arg);
  ret = phPalEse_ioctl(phPalEse_e_SetClientUpdateState, pEseDeviceHandle, arg);
  if (ret < 0) {
    NXP_LOG_ESE_E("%s : failed errno = 0x%x", __FUNCTION__, errno);
    status = ESESTATUS_FAILED;
  }

  return status;
}
