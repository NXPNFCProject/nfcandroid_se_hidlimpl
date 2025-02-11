/******************************************************************************
 *
 *  Copyright 2018-2019, 2023-2024 NXP
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
 * \addtogroup SPI_Power_Management
 *
 * @{ */

#ifndef _PHNXPESE_SPM_H
#define _PHNXPESE_SPM_H

#include <phEseStatus.h>
#include <phNxpEseFeatures.h>
/*! SPI Power Manager (SPM) possible error codes */
#define SPM_RECOVERY_RESET 5

/**
 * \ingroup SPI_Power_Management
 * \brief This function opens the nfc i2c driver to manage power
 *                  and synchronization for ese secure element.
 *
 * \param[in]    pDevHandle       - Device handle to open.
 *
 * \retval       -On Success ESESTATUS_SUCCESS else proper error code
 */
ESESTATUS phNxpEse_SPM_Init(void* pDevHandle);

/**
 * \ingroup SPI_Power_Management
 * \brief TThis function closes the nfc i2c driver node.
 *
 *
 * \retval       -On Success ESESTATUS_SUCCESS else proper error code
 */
ESESTATUS phNxpEse_SPM_DeInit(void);

/**
 * \ingroup SPI_Power_Management
 * \brief This function request to the nfc i2c driver
 * to enable/disable power to ese. This api should be called
 *before sending any apdu to ese/once apdu exchange is done.
 *
 * \param[in]    arg       -input can be of  type int.
 *
 * \retval       -On Success ESESTATUS_SUCCESS else proper error code
 */
ESESTATUS phNxpEse_SPM_ConfigPwr(int arg);

/**
 * \ingroup SPI_Power_Management
 * \brief   This function is used to set the ese Update state.
 *
 * \param[in]    arg - eSE update status started/completed.
 *
 * \retval       -On Success ESESTATUS_SUCCESS else proper error code
 */
ESESTATUS phNxpEse_SPM_SetEseClientUpdateState(long arg);

#endif /*  _PHNXPESE_SPM_H    */
/** @} */
