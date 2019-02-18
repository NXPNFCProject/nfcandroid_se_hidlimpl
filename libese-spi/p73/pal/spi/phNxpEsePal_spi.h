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

/**
 * \addtogroup eSe_PAL_Spi
 * \brief PAL SPI port implementation for linux
 * @{ */
#ifndef _PHNXPESE_PAL_SPI_H
#define _PHNXPESE_PAL_SPI_H

/* Basic type definitions */
#include <phEseTypes.h>
#include <phNxpEsePal.h>

/*!
 * \brief Start of frame marker
 */
#define SEND_PACKET_SOF 0x5A
/*!
 * \brief ESE Poll timeout (max 1.3 seconds)
 */
#define ESE_POLL_TIMEOUT (2000)
/*!
 * \brief ESE Max Poll retry count
 */
#define ESE_NAD_POLLING_MAX (1000)

/*!
 * \brief ESE wakeup delay in case of write error retry
 */
#define WAKE_UP_DELAY 100
/*!
 * \brief ESE wakeup delay in case of write error retry
 */
#define NAD_POLLING_SCALER 10
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
#define P61_SET_PWR _IOW(P61_MAGIC, 0x01, long)
/*!
 * \brief IOCTL number to set debug state
 */
#define P61_SET_DBG _IOW(P61_MAGIC, 0x02, long)
/*!
 * \brief IOCTL number to enable poll mode
 */
#define P61_SET_POLL _IOW(P61_MAGIC, 0x03, long)
/*!
 * \brief SPI Request NFCC to enable p61 power, only in param
 *         Only for SPI
 *         level 1 = Enable power
 *         level 0 = Disable power
 */
#define P61_SET_SPM_PWR _IOW(P61_MAGIC, 0x04, long)

/*!
 * \brief SPI or DWP can call this ioctl to get the current
 *         power state of P61
 *
 */
#define P61_GET_SPM_STATUS _IOR(P61_MAGIC, 0x05, long)
/*!
 * \brief IOCTL to add throughput measurment source code in device driver
 *
 */
#define P61_SET_THROUGHPUT _IOW(P61_MAGIC, 0x06, long)
/*!
 * \brief IOCTL to get the ESE access
 *
 */
#define P61_GET_ESE_ACCESS _IOW(P61_MAGIC, 0x07, long)
/*!
 * \brief IOCTL to set the power scheme
 *
 */
#define P61_SET_POWER_SCHEME _IOW(P61_MAGIC, 0x08, long)
/*!
 * \brief This function is used to set the ESE jcop
 *  download state.
 */
#define P61_SET_DWNLD_STATUS _IOW(P61_MAGIC, 0x09, long)

/*!
 * \brief This function is used to set disable ESE GPIO
 *  state On&Off
 */
#define P61_INHIBIT_PWR_CNTRL _IOW(P61_MAGIC, 0x0A, long)
/*!
 * \brief IOCTL to set the GPIO for the eSE to distinguish
 *        the logical interface
 */
#define ESE_SET_TRUSTED_ACCESS  _IOW(P61_MAGIC, 0x0B, long)

/*!
 * \brief IOCTL to perform the eSE COLD_RESET  via NFC driver.
 */
#define ESE_PERFORM_COLD_RESET  _IOW(P61_MAGIC, 0x0C, long)

/* Function declarations */
/**
 * \ingroup eSe_PAL_Spi
 * \brief This function is used to close the ESE device
 *
 * \retval None
 *
 */

void phPalEse_spi_close(void* pDevHandle);

/**
 * \ingroup eSe_PAL_Spi
 * \brief Open and configure ESE device
 *
 * \param[in]       pphPalEse_Config_t: Config to open the device
 *
 * \retval  ESESTATUS On Success ESESTATUS_SUCCESS else proper error code
 *
 */
ESESTATUS phPalEse_spi_open_and_configure(pphPalEse_Config_t pConfig);

/**
 * \ingroup eSe_PAL_Spi
 * \brief Reads requested number of bytes from ESE into given buffer
 *
 * \param[in]    pDevHandle       - valid device handle
 **\param[in]    pBuffer          - buffer for read data
 **\param[in]    nNbBytesToRead   - number of bytes requested to be read
 *
 * \retval   numRead      - number of successfully read bytes.
 * \retval      -1             - read operation failure
 *
 */
int phPalEse_spi_read(void* pDevHandle, uint8_t* pBuffer, int nNbBytesToRead);

/**
 * \ingroup eSe_PAL_Spi
 * \brief Writes requested number of bytes from given buffer into pn547 device
 *
 * \param[in]    pDevHandle               - valid device handle
 * \param[in]    pBuffer                     - buffer to write
 * \param[in]    nNbBytesToWrite       - number of bytes to write
 *
 * \retval  numWrote   - number of successfully written bytes
 * \retval      -1         - write operation failure
 *
 */
int phPalEse_spi_write(void* pDevHandle, uint8_t* pBuffer, int nNbBytesToWrite);

/**
 * \ingroup eSe_PAL_Spi
 * \brief Exposed ioctl by ESE driver
 *
 * \param[in]    eControlCode       - phPalEse_ControlCode_t for the respective
 *configs
 * \param[in]    pDevHandle           - valid device handle
 * \param[in]    pBuffer              - buffer for read data
 * \param[in]    level                  - reset level
 *
 * \retval    0   - ioctl operation success
 * \retval   -1  - ioctl operation failure
 *
 */
ESESTATUS phPalEse_spi_ioctl(phPalEse_ControlCode_t eControlCode, void* pDevHandle,
                       long level);

/**
 * \ingroup eSe_PAL_Spi
 * \brief Print packet data
 *
 * \param[in]    pString           - String to be printed
 * \param[in]    p_data               - data to be printed
 * \param[in]    len                  - Length of data to be printed
 *
 * \retval   void
 *
 */
void phPalEse_spi_print_packet(const char* pString, const uint8_t* p_data,
                               uint16_t len);
/**
 * \ingroup eSe_PAL_Spi
 * \brief This function  suspends execution of the calling thread for
 *                  (at least) usec microseconds
 *
 * \param[in]    usec           - number of micro seconds to sleep
 *
 * \retval   void
 *
 */
void phPalEse_spi_sleep(uint32_t usec);


/** @} */
#endif /*  _PHNXPESE_PAL_SPI_H    */
