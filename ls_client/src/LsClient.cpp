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

#include "LsLib.h"
#include "LsClient.h"
#include <cutils/log.h>
#include <dirent.h>
#include <stdlib.h>
#include <pthread.h>

/*static char gethex(const char *s, char **endptr);
char *convert(const char *s, int *length);*/
uint8_t datahex(char c);
extern pLsc_Dwnld_Context_t gpLsc_Dwnld_Context;
static android::sp<ISecureElementHalCallback> cCallback;
/*******************************************************************************
**
** Function:        LSC_Start
**
** Description:     Starts the LSC update over DWP
**
** Returns:         SUCCESS if ok.
**
*******************************************************************************/
tLSC_STATUS LSC_Start(const char* name, const char* dest, uint8_t* pdata,
                      uint16_t len, uint8_t* respSW) {
  static const char fn[] = "LSC_Start";
  tLSC_STATUS status = STATUS_FAILED;
  if (name != NULL) {
    ALOGE("%s: name is %s", fn, name);
    ALOGE("%s: Dest is %s", fn, dest);
    status = Perform_LSC(name, dest, pdata, len, respSW);
  } else {
    ALOGE("Invalid parameter");
  }
  ALOGE("%s: Exit; status=0x0%X", fn, status);
  return status;
}

/*******************************************************************************
**
** Function:        performLSDownload
**
** Description:     Perform LS during hal init
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
tLSC_STATUS performLSDownload(
    const android::sp<ISecureElementHalCallback>& clientCallback) {
  tLSC_STATUS status = STATUS_FAILED;
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  cCallback = clientCallback;
  if (pthread_create(&thread, &attr, &performLSDownload_thread, NULL) != 0) {
    ALOGD("Thread creation failed");
    status = STATUS_FAILED;
  } else {
    status = STATUS_SUCCESS;
    ALOGD("Thread creation success");
  }
  pthread_attr_destroy(&attr);
  return status;
}

/*******************************************************************************
**
** Function:        performLSDownload
**
** Description:     Perform LS during hal init
**
** Returns:         None
**
*******************************************************************************/
void* performLSDownload_thread(void* data) {
  ALOGD("%s enter:  ", __func__);
  (void)data;
  tLSC_STATUS status;

  const char* lsUpdateBackupPath =
      "/data/vendor/secure_element/loaderservice_updater.txt";
  const char* lsUpdateBackupOutPath =
      "/data/vendor/secure_element/loaderservice_updater_out.txt";

  /*generated SHA-1 string for secureElementLS
  This will remain constant as handled in secureElement HAL*/
  char sha1[] = "6d583e84f2710e6b0f06beebc1a12a1083591373";
  uint8_t hash[20] = {};

  for (int i = 0; i < 40; i = i + 2) {
    hash[i / 2] =
        (((datahex(sha1[i]) & 0x0F) << 4) | (datahex(sha1[i + 1]) & 0x0F));
  }

  gpLsc_Dwnld_Context =
      (pLsc_Dwnld_Context_t)malloc(sizeof(Lsc_Dwnld_Context_t));
  if (gpLsc_Dwnld_Context != NULL) {
    memset((void*)gpLsc_Dwnld_Context, 0,
           (uint32_t)sizeof(Lsc_Dwnld_Context_t));
  } else {
    ALOGD("%s: Memory allocation failed", __func__);
  }

  uint8_t resSW[4] = {0x4e, 0x02, 0x69, 0x87};
  FILE* fIn, *fOut;
  if ((fIn = fopen(lsUpdateBackupPath, "rb")) == NULL) {
    ALOGE("%s Cannot open file %s\n", __func__, lsUpdateBackupPath);
    ALOGE("%s Error : %s", __func__, strerror(errno));
    cCallback->onStateChange(true);
  } else {
    ALOGD("%s File opened %s\n", __func__, lsUpdateBackupPath);
    fseek(fIn, 0, SEEK_END);
    long fsize = ftell(fIn);
    rewind(fIn);

    char* lsUpdateBuf = (char*)malloc(fsize + 1);
    fread(lsUpdateBuf, fsize, 1, fIn);

    if ((fOut = fopen(lsUpdateBackupOutPath, "wb")) == NULL) {
      ALOGE("%s Failed to open file %s\n", __func__, lsUpdateBackupOutPath);
    } else {
      ALOGD("%s File opened %s\n", __func__, lsUpdateBackupOutPath);
    }

    if ((long)fwrite(lsUpdateBuf, 1, fsize, fOut) != fsize) {
      ALOGE("%s ERROR - Failed to write %ld bytes to file\n", __func__, fsize);
    }

    status = LSC_Start(lsUpdateBackupPath, lsUpdateBackupOutPath,
                       (uint8_t*)hash, (uint16_t)sizeof(hash), resSW);
    ALOGD("%s LSC_Start completed\n", __func__);
    if (status == STATUS_SUCCESS) {
      if (remove(lsUpdateBackupPath) == 0) {
        ALOGD("%s  : %s file deleted successfully\n", __func__,
              lsUpdateBackupPath);
      } else {
        ALOGD("%s  : %s file deletion failed!!!\n", __func__,
              lsUpdateBackupPath);
      }
      cCallback->onStateChange(true);
    }
    free(lsUpdateBuf);
  }
  ALOGD("%s pthread_exit\n", __func__);
  pthread_exit(NULL);
  return NULL;
}

/*******************************************************************************
**
** Function:        datahex
**
** Description:     Converts char to uint8_t
**
** Returns:         uint8_t variable
**
*******************************************************************************/
uint8_t datahex(char c) {
  uint8_t value = 0;
  if (c >= '0' && c <= '9')
    value = (c - '0');
  else if (c >= 'A' && c <= 'F')
    value = (10 + (c - 'A'));
  else if (c >= 'a' && c <= 'f')
    value = (10 + (c - 'a'));
  return value;
}
