/******************************************************************************
 *
 *  Copyright 2018-2019,2024-2025 NXP
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
#ifndef _PHNXPSPILIB_H_
#define _PHNXPSPILIB_H_

#include <phNxpEse_Api.h>

/* Macro to enable SPM Module */
#define SPM_INTEGRATED
//#undef SPM_INTEGRATED
#ifdef SPM_INTEGRATED
#include "../spm/phNxpEse_Spm.h"
#endif

/********************* Definitions and structures *****************************/

typedef enum {
  ESE_STATUS_CLOSE = 0x00,
  ESE_STATUS_BUSY,
  ESE_STATUS_RECOVERY,
  ESE_STATUS_IDLE,
  ESE_STATUS_OPEN,
} phNxpEse_LibStatus;

typedef enum {
  END_POINT_ESE = 0, /*!< eSE services */
  END_POINT_EUICC,   /*!< UICC services*/
  MAX_END_POINTS
} phNxpEse_EndPoint;

/* Macros definition */
#define MAX_DATA_LEN 780
#define SECOND_TO_MILLISECOND(X) X * 1000
#define CONVERT_TO_PERCENTAGE(X, Y) X* Y / 100
#define ADDITIONAL_SECURE_TIME_PERCENTAGE 5

/*!
 * \brief  Secure timer values F1, F2, F3
 *
 *
 */
typedef struct phNxpEse_SecureTimer {
  unsigned int secureTimer1; /*!< Secure timer 1 value */
  unsigned int secureTimer2; /*!< Secure timer 2 value */
  unsigned int secureTimer3; /*!< Secure timer 3 value */
} phNxpEse_SecureTimer_t;

typedef enum nadInfoTx {
  ESE_NAD_TX = 0x5A,  /*!< R-frame Acknowledgement frame indicator */
  EUICC_NAD_TX = 0x4B /*!< R-frame Negative-Acknowledgement frame indicator */
} nadInfoTx_t;

/*!
 * \brief R-Frame types used in 7816-3 protocol stack
 */
typedef enum nadInfoRx {
  ESE_NAD_RX = 0xA5,  /*!< R-frame Acknowledgement frame indicator */
  EUICC_NAD_RX = 0xB4 /*!< R-frame Negative-Acknowledgement frame indicator */
} nadInfoRx_t;

/*!
 * \brief  Node address Info structure
 *
 *
 */
typedef struct phNxpEseNadInfo {
  nadInfoTx_t nadTx; /*!< nod address for tx */
  nadInfoRx_t nadRx; /*!< nod address for rx */
} phNxpEseNadInfo_t;

/*!
 * \brief  SPI Control structure
 *
 *
 */
typedef struct phNxpEse_Context {
  void* pDevHandle;                /*!<device handle */
  long nadPollingRetryTime;        /*!<polling retry for nod address */
  long invalidFrame_Rnack_Delay;   /*!<delay before retrying when rnack is
                                      received */
  phNxpEse_LibStatus EseLibStatus; /*!<Indicate if Ese Lib is open or closed */
  phNxpEse_initParams initParams;  /*!<init params */
  phNxpEse_SecureTimer_t secureTimerParams; /*!<secure timer params */
  phNxpEseNadInfo_t nadInfo;                /*!<nad info */
  uint8_t p_read_buff[MAX_DATA_LEN];        /*!<read buffer */
  uint8_t p_cmd_data[MAX_DATA_LEN];         /*!<cmd  buffer */
  uint16_t cmd_len;                         /*!<cmd buffer length */
  uint8_t endPointInfo;                     /*!<info end point*/
  bool rnack_sent;                          /*!<rnack send info */
  NotifyWtxReq* fPtr_WtxNtf; /*!< Wait extension callback notification*/
} phNxpEse_Context_t;

/* Timeout value to wait for response from
   Note: Timeout value updated from 1000 to 2000 to fix the JCOP delay (WTX)*/
#define HAL_EXTNS_WRITE_RSP_TIMEOUT (2000)

#define SPILIB_CMD_CODE_LEN_BYTE_OFFSET (2U)
#define SPILIB_CMD_CODE_BYTE_LEN (3U)

static nadInfoTx_t nadInfoTx_ptr[MAX_END_POINTS] = {ESE_NAD_TX, EUICC_NAD_TX};

static nadInfoRx_t nadInfoRx_ptr[MAX_END_POINTS] = {ESE_NAD_RX, EUICC_NAD_RX};
ESESTATUS phNxpEse_WriteFrame(uint32_t data_len, uint8_t* p_data);
ESESTATUS phNxpEse_read(uint32_t* data_len, uint8_t** pp_data);
void phNxpEse_setOsVersion(phNxpEse_OsVersion_t chipType);

#endif /* _PHNXPSPILIB_H_ */
