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

#define LOG_TAG "StateMachine"

#include <NfcAdaptation.h>
#include <hal_nxpese.h>
#include <log/log.h>
#include <stdint.h>
#include <string.h>

#include "EseHalStates.h"
#include "StateMachine.h"

#define HAL_NFC_SPI_DWP_SYNC 21

extern int omapi_status;
bool state_machine_debug = true;

map<eStates_t, StateBase *> StateBase::sListOfStates;

class StateSpiBusyRfBusy : public StateBase {
public:
  StateSpiBusyRfBusy() {}
  ~StateSpiBusyRfBusy() {}

  eStates_t GetState() { return ST_SPI_BUSY_RF_BUSY; }

  StateBase *ProcessEvent(eExtEvent_t event) {
    StateBase *PtrNextState = this;
    switch (event) {
    case EVT_RF_ON:
      break;
    case EVT_RF_OFF:
      SendOMAPISessionOpenCmd();
      PtrNextState = sListOfStates.find(ST_SPI_BUSY_RF_IDLE)->second;
      break;
    case EVT_SPI_RX:
      PtrNextState = sListOfStates.find(ST_SPI_OPEN_RESUMED_RF_BUSY)->second;
      break;
    default:
      break;
    }
    return PtrNextState;
  }
};

class StateSpiBusyRfIdle : public StateBase {
public:
  StateSpiBusyRfIdle() {}
  ~StateSpiBusyRfIdle() {}

  eStates_t GetState() { return ST_SPI_BUSY_RF_IDLE; }

  StateBase *ProcessEvent(eExtEvent_t event) {
    StateBase *PtrNextState = this;
    switch (event) {
    case EVT_RF_ON:
      PtrNextState = sListOfStates.find(ST_SPI_RX_PENDING_RF_IDLE)->second;
      break;
    case EVT_SPI_RX:
      PtrNextState = sListOfStates.find(ST_SPI_OPEN_RF_IDLE)->second;
      break;
    default:
      break;
    }
    return PtrNextState;
  }
};

class StateSpiClosedRfBusy : public StateBase {
public:
  StateSpiClosedRfBusy() {}
  ~StateSpiClosedRfBusy() {}

  eStates_t GetState() { return ST_SPI_CLOSED_RF_BUSY; }

  StateBase *ProcessEvent(eExtEvent_t event) {
    StateBase *PtrNextState = this;
    switch (event) {
    case EVT_RF_ON:
      break;
    case EVT_RF_OFF:
      PtrNextState = sListOfStates.find(ST_SPI_CLOSED_RF_IDLE)->second;
      break;
    case EVT_SPI_OPEN:
    case EVT_SPI_CLOSE:
      break;
    default:
      break;
    }
    return PtrNextState;
  }
};

class StateSpiClosedRfIdle : public StateBase {
public:
  StateSpiClosedRfIdle() {}
  ~StateSpiClosedRfIdle() {}

  eStates_t GetState() { return ST_SPI_CLOSED_RF_IDLE; }

  StateBase *ProcessEvent(eExtEvent_t event) {
    StateBase *PtrNextState = this;
    switch (event) {
    case EVT_SPI_HW_SERVICE_START:
      break;
    case EVT_RF_ON:
      PtrNextState = sListOfStates.find(ST_SPI_CLOSED_RF_BUSY)->second;
      break;
    case EVT_SPI_OPEN:
      // TODO: Can OMAPI session cmd be send from here?
      PtrNextState = sListOfStates.find(ST_SPI_OPEN_RF_IDLE)->second;
      break;
    default:
      break;
    }
    return PtrNextState;
  }
};

class StateSpiOpenResumedRfBusy : public StateBase {
public:
  StateSpiOpenResumedRfBusy() {}
  ~StateSpiOpenResumedRfBusy() {}

  eStates_t GetState() { return ST_SPI_OPEN_RESUMED_RF_BUSY; }

  StateBase *ProcessEvent(eExtEvent_t event) {
    StateBase *PtrNextState = this;
    switch (event) {
    case EVT_RF_ON:
      break;
    case EVT_RF_OFF:
      SendOMAPISessionOpenCmd();
      PtrNextState = sListOfStates.find(ST_SPI_OPEN_RF_IDLE)->second;
      break;
    case EVT_SPI_TX:
      PtrNextState = sListOfStates.find(ST_SPI_BUSY_RF_BUSY)->second;
      break;
    case EVT_SPI_CLOSE:
      PtrNextState = sListOfStates.find(ST_SPI_CLOSED_RF_BUSY)->second;
      break;
    default:
      break;
    }
    return PtrNextState;
  }
};

class StateSpiOpenRfIdle : public StateBase {
public:
  StateSpiOpenRfIdle() {}
  ~StateSpiOpenRfIdle() {}

  eStates_t GetState() { return ST_SPI_OPEN_RF_IDLE; }

  StateBase *ProcessEvent(eExtEvent_t event) {
    StateBase *PtrNextState = this;
    switch (event) {
    case EVT_RF_ON:
      PtrNextState = sListOfStates.find(ST_SPI_OPEN_SUSPENDED_RF_BUSY)->second;
      SendSwpSwitchAllowCmd();
      break;
    case EVT_SPI_TX:
      PtrNextState = sListOfStates.find(ST_SPI_BUSY_RF_IDLE)->second;
      break;
    case EVT_SPI_CLOSE:
      PtrNextState = sListOfStates.find(ST_SPI_CLOSED_RF_IDLE)->second;
      break;
    default:
      break;
    }
    return PtrNextState;
  }
};

class StateSpiOpenSuspendedRfBusy : public StateBase {
public:
  StateSpiOpenSuspendedRfBusy() {}
  ~StateSpiOpenSuspendedRfBusy() {}

  eStates_t GetState() { return ST_SPI_OPEN_SUSPENDED_RF_BUSY; }

  StateBase *ProcessEvent(eExtEvent_t event) {
    StateBase *PtrNextState = this;
    switch (event) {
    case EVT_RF_ON:
      break;
    case EVT_RF_OFF:
      SendOMAPISessionOpenCmd();
      PtrNextState = sListOfStates.find(ST_SPI_OPEN_RF_IDLE)->second;
      break;
    case EVT_RF_ACT_NTF_ESE_F:
      PtrNextState = sListOfStates.find(ST_SPI_OPEN_RESUMED_RF_BUSY)->second;
      break;
    case EVT_SPI_TX:
      break;
    case EVT_SPI_CLOSE:
      PtrNextState = sListOfStates.find(ST_SPI_CLOSED_RF_BUSY)->second;
      break;
    default:
      break;
    }
    return PtrNextState;
  }
};

class StateSpiRxPendingRfIdle : public StateBase {
public:
  StateSpiRxPendingRfIdle() {}
  ~StateSpiRxPendingRfIdle() {}

  eStates_t GetState() { return ST_SPI_RX_PENDING_RF_IDLE; }

  StateBase *ProcessEvent(eExtEvent_t event) {
    StateBase *PtrNextState = this;
    switch (event) {
    case EVT_SPI_RX:
      PtrNextState = sListOfStates.find(ST_SPI_OPEN_SUSPENDED_RF_BUSY)->second;
      break;
    default:
      break;
    }
    return PtrNextState;
  }
};

/************************State Base Class Definition***************************/

StateBase *StateBase::InitializeStates() {
  StateBase::sListOfStates.insert(
      make_pair(ST_SPI_CLOSED_RF_IDLE, new StateSpiClosedRfIdle()));
  StateBase::sListOfStates.insert(
      make_pair(ST_SPI_CLOSED_RF_BUSY, new StateSpiClosedRfBusy()));
  StateBase::sListOfStates.insert(
      make_pair(ST_SPI_OPEN_RF_IDLE, new StateSpiOpenRfIdle()));
  StateBase::sListOfStates.insert(
      make_pair(ST_SPI_BUSY_RF_IDLE, new StateSpiBusyRfIdle()));
  StateBase::sListOfStates.insert(
      make_pair(ST_SPI_RX_PENDING_RF_IDLE, new StateSpiRxPendingRfIdle()));
  StateBase::sListOfStates.insert(make_pair(ST_SPI_OPEN_SUSPENDED_RF_BUSY,
                                            new StateSpiOpenSuspendedRfBusy()));
  StateBase::sListOfStates.insert(
      make_pair(ST_SPI_OPEN_RESUMED_RF_BUSY, new StateSpiOpenResumedRfBusy()));
  StateBase::sListOfStates.insert(
      make_pair(ST_SPI_BUSY_RF_BUSY, new StateSpiBusyRfBusy()));

  StateBase *PtrCurrentState =
      sListOfStates.find(ST_SPI_CLOSED_RF_IDLE)->second;
  return PtrCurrentState;
}

eStatus_t StateBase::SendOMAPICommand(uint8_t cmd[], uint8_t cmd_len) {
  int nfc_access_retryCnt = 0;
  int retval;
  ese_nxp_IoctlInOutData_t inpOutData;
  memset(&inpOutData, 0x00, sizeof(ese_nxp_IoctlInOutData_t));
  inpOutData.inp.data.nxpCmd.cmd_len = cmd_len;
  inpOutData.inp.data_source = 1;
  memcpy(inpOutData.inp.data.nxpCmd.p_cmd, cmd, cmd_len);
retry_nfc_access:
  retval =
      NfcAdaptation::GetInstance().HalIoctl(HAL_NFC_SPI_DWP_SYNC, &inpOutData);
  if (omapi_status != 0) {
    ALOGE_IF(state_machine_debug, "omapi_status return failed");
    nfc_access_retryCnt++;
    usleep(2000000);
    if (nfc_access_retryCnt < 5)
      goto retry_nfc_access;
    return SM_STATUS_FAILED;
  }

  return SM_STATUS_SUCCESS;
}

eStatus_t StateBase::SendOMAPISessionOpenCmd() {
  uint8_t cmd_omapi_session_open[] = {0x2F, 0x01, 0x01, 0x01};
  return SendOMAPICommand(cmd_omapi_session_open,
                          sizeof(cmd_omapi_session_open));
}

eStatus_t StateBase::SendOMAPISessionCloseCmd() {
  uint8_t cmd_omapi_session_close[] = {0x2F, 0x01, 0x01, 0x00};
  return SendOMAPICommand(cmd_omapi_session_close,
                          sizeof(cmd_omapi_session_close));
}

eStatus_t StateBase::SendSwpSwitchAllowCmd() {
  uint8_t cmd_clt_session_allow[] = {0x2F, 0x01, 0x01, 0x02};
  return SendOMAPICommand(cmd_clt_session_allow, sizeof(cmd_clt_session_allow));
}
