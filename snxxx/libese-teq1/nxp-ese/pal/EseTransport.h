/******************************************************************************
 *
 *  Copyright 2020-2022,2024-2025 NXP
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

/* Basic type definitions */
#include <phNxpEsePal.h>

/*!
 * \brief Start of frame marker
 */
#define SEND_PACKET_SOF 0x5A
/*!
 * \brief ESE Poll timeout (max 2 seconds)
 */
#define ESE_POLL_TIMEOUT (2 * 1000)
/*!
 * \brief ESE Max Poll retry count
 */

#define ESE_NAD_POLLING_MAX (2000)

/*!
 * \brief ESE wakeup delay in case of write error retry
 */

#define WAKE_UP_DELAY_USECS 100

#define GET_WAKE_UP_DELAY() (WAKE_UP_DELAY_USECS)

/*!
 * \brief ESE wakeup delay in case of write error retry
 */

#define NAD_POLLING_SCALER 1

/*!
 * \brief ESE wakeup delay in case of write error retry
 */
#define CHAINED_PKT_SCALER 1
/*!
 * \brief Magic type specific to the ESE device driver
 */
#define P61_MAGIC 0xEA

/*!
 * \brief IOCTL number to set ESE PWR
 */
#define P61_SET_PWR _IOW(P61_MAGIC, 0x01, uint64_t)
/*!
 * \brief IOCTL number to set debug state
 */
#define P61_SET_DBG _IOW(P61_MAGIC, 0x02, uint64_t)
/*!
 * \brief IOCTL to set the GPIO for the eSE to distinguish
 *        the logical interface
 */
#define ESE_SET_TRUSTED_ACCESS _IOW(P61_MAGIC, 0x0B, uint64_t)

/*!
 * \brief IOCTL to perform the eSE COLD_RESET  via NFC driver.
 */
#define ESE_PERFORM_COLD_RESET _IOW(P61_MAGIC, 0x0C, uint64_t)
/*!
 * \brief IOCTL to enable/disable GPIO/COLD reset protection.
 */
#define PERFORM_RESET_PROTECTION _IOW(P61_MAGIC, 0x0D, uint64_t)

class EseTransport {
 public:
  virtual void Close(void* pDevHandle) = 0;
  virtual ESESTATUS OpenAndConfigure(pphPalEse_Config_t pConfig) = 0;
  virtual int Read(void* pDevHandle, uint8_t* pBuffer, int nNbBytesToRead) = 0;
  virtual int Write(void* pDevHandle, uint8_t* pBuffer,
                    int nNbBytesToWrite) = 0;
  virtual ESESTATUS Ioctl(phPalEse_ControlCode_t eControlCode, void* pDevHandle,
                          long level) = 0;
  virtual ~EseTransport(){};
};
