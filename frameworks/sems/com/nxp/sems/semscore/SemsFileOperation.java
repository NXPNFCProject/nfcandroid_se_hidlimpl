/*
 * Copyright 2022,2025-2026 NXP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.nxp.sems;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.util.Log;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

public class SemsFileOperation {
  public static final String TAG = "SEMS-SemsFileOperation";
  // Use StringBuilder instead of String for log accumulation
  private StringBuilder mRespOutlogBuilder;
  private String mEncryptedScriptDirectory = "";
  private String mOutDirectory = "";
  String mCallerPackageName;

  private static final byte SEMS_RESPONSE = 0x01;
  private static final byte ERROR_RESPONSE = 0x03;
  private static final byte SEMS_CERT_RESPONSE = 0x05;
  private static final byte SEMS_AUTH_RESPONSE = 0x06;
  private static final byte SEMS_RESPONSE_DATA_TAG = 0x43;
  private static final byte SEMS_FRAME_TYPE_TAG = 0x44;
  private static final byte SEMS_RESPONSE_LOG_TAG = 0x61;

  // Pre-allocated static byte arrays to avoid repeated allocations
  private static final byte[] CERT_RESPONSE_DATA = {0x7F, 0x21};
  private static final byte[] AUTH_RESPONSE_DATA = {0x60};
  private static final byte[] RESPONSE_DATA = {0x40};
  // Reusable DateTimeFormatter
  private static final DateTimeFormatter DATE_TIME_FORMATTER =
      DateTimeFormatter.ofPattern("yyyy/MM/dd HH:mm:ss");
  // Constants for string literals
  private static final String TIMESTAMP_PREFIX = "#######";
  private static final String TIMESTAMP_SUFFIX = "#######\r\n";
  private static final String LINE_SEPARATOR = "\r\n";
  private static final String METADATA_PREFIX = "%%%";

  /**
   * SemsFileOperation Constructor
   * <br/>
   * @param void
   *
   * @return void.
   */
  SemsFileOperation() {
    // Pre-allocate StringBuilder with reasonable initial capacity
    mRespOutlogBuilder = new StringBuilder(8192);
    // Update start time stamp at beginning
    appendCurrentTimeStamp();
  }

  /**
   * Get current date and time stamp
   * <br/>
   * Used in response log for debug purpose
   * @param void
   *
   * @return String current data and time stamp.
   */
  private String getCurrentTimeStamp() {
    LocalDateTime now = LocalDateTime.now();
    return TIMESTAMP_PREFIX + DATE_TIME_FORMATTER.format(now) + TIMESTAMP_SUFFIX;
  }

  /**
   * Append current timestamp to log builder
   * <br/>
   * Used in response log for debug purpose
   * @param void
   *
   * @return void
   */
  private void appendCurrentTimeStamp() {
    LocalDateTime now = LocalDateTime.now();
    mRespOutlogBuilder.append(TIMESTAMP_PREFIX)
                      .append(DATE_TIME_FORMATTER.format(now))
                      .append(TIMESTAMP_SUFFIX);
  }

  /**
   * Logging the response APDU received during SEMS execution
   * <br/>
   * Agent to provide the SEMS Application with an identifier
   * @param byte[] what The Input status bytes
   *        Byte type The Input type of response
   *
   * @return void.
   */
  public void putIntoLog(byte[] what, byte type) {
     // Early return optimization - check type first (cheaper than array access)
    if (type == ERROR_RESPONSE) {
      return;
    }
    // Early return for SW "6310" check
    int len = what.length;
    if (len >= 2 && what[len - 2] == 0x63 && what[len - 1] == 0x10) {
      return;
    }
    // Use pre-allocated static byte arrays
    byte[] data;
    switch (type) {
      case SEMS_CERT_RESPONSE:
        data = CERT_RESPONSE_DATA;
        break;
      case SEMS_AUTH_RESPONSE:
        data = AUTH_RESPONSE_DATA;
        break;
      case SEMS_RESPONSE:
        data = RESPONSE_DATA;
        break;
      default:
        return;
    }
    // Build TLV structure
    byte[] innerTLV = SemsUtil.append(
        SemsTLV.make(SEMS_RESPONSE_DATA_TAG, data),
        SemsTLV.make(SEMS_RESPONSE_DATA_TAG, what)
    );
    byte[] outerTLV = SemsTLV.make(SEMS_RESPONSE_LOG_TAG, innerTLV);
    // Append to StringBuilder instead of string concatenation
    mRespOutlogBuilder.append(SemsUtil.toHexString(outerTLV))
                      .append(LINE_SEPARATOR);
  }

  /**
   * Set the current application directory & caller information
   * <br/>
   * @param context caller application context info
   *
   * @return {@code SemsStatus} returns SEMS_STATUS_SUCCESS on success
   * otherwise SEMS_STATUS_FAILED.
   */
  public SemsStatus setDirectories(Context context) {
   SemsStatus status = SemsStatus.SEMS_STATUS_FAILED;
    PackageManager pm = context.getPackageManager();
    String packageName = context.getPackageName();

    synchronized (SemsFileOperation.class) {
      try {
        PackageInfo pInfo = pm.getPackageInfo(packageName, 0);
        mEncryptedScriptDirectory = pInfo.applicationInfo.dataDir;
        mOutDirectory = pInfo.applicationInfo.dataDir;
        mCallerPackageName = pInfo.packageName;
        status = SemsStatus.SEMS_STATUS_SUCCESS;
      } catch (PackageManager.NameNotFoundException e) {
        Log.e(TAG, "Package not found: " + packageName, e);
      }
    }
    return status;
  }

  /**
   * Get path to locate and access file
   * <br/>
   * @param String dir initial part of path string
   *        String file filename in the path
   *
   * @return Path The resulting path or null if file handle is null.
   */
  public Path getPath(String dir, String file) {
    if (file == null) {
      Log.e(TAG, "getPath: file is null");
      return null;
    }
    return (dir != null) ? FileSystems.getDefault().getPath(dir, file)
                         : FileSystems.getDefault().getPath(file);
  }

  /**
   * Write the content of accumulated response buffer to out file
   * <br/>
   * Agent to provide the SEMS Application with an identifier
   * @param String scriptOut The file name to write response
   *
   * @return byte[] Response byte array or null if invalid input.
   */
  public byte[] writeScriptOutFile(String scriptOut) {
    if (scriptOut == null) {
      Log.e(TAG, "writeScriptOutFile: scriptOut is null");
      return null;
    }
    Path p = getPath(mOutDirectory, scriptOut);
    // Update finish time stamp at end
    appendCurrentTimeStamp();
    // Convert to bytes once
    byte[] outputBytes = mRespOutlogBuilder.toString().getBytes(StandardCharsets.UTF_8);
    try {
      Files.write(p, outputBytes);
    } catch (IOException e) {
      Log.e(TAG, "IOException during writeScriptOutfile", e);
    }
    return outputBytes;
  }

  /**
   * Write the content of String buffer to backup file
   * <br/>
   * @param String filename to which the buffer contents to be copied
   *        String scriptBuffer input buffer content
   *
   * @return byte[] response buffer or null if input is invalid.
   */
  public byte[] writeScriptInputFile(String filename, String scriptBuffer) {
    if (scriptBuffer == null || filename == null) {
      Log.e(TAG, "writeScriptInputFile: scriptBuffer or filename is null");
      return null;
    }

    Path p = getPath(mOutDirectory, filename);
    byte[] bufferBytes = scriptBuffer.getBytes(StandardCharsets.UTF_8);

    try {
      Files.write(p, bufferBytes);
    } catch (IOException e) {
      Log.e(TAG, "IOException during writeScriptInputFile", e);
    }
    return bufferBytes;
  }

  /**
   * Read the file content to String format
   * <br/>
   * The Input path of the SEMS encrypted script stored,
   * @param String scriptIn input SEMS script with metadata
   *
   * @return String plain script with metadata removed.
   */
  public String readScriptFile(String scriptIn) throws Exception {
    Path p = getPath(mEncryptedScriptDirectory, scriptIn);
    StringBuilder script = new StringBuilder(4096);

    try (BufferedReader reader = Files.newBufferedReader(p, StandardCharsets.UTF_8)) {
      String line;
      while ((line = reader.readLine()) != null) {
        if (!line.startsWith(METADATA_PREFIX)) {
          script.append(line);
        }
      }
    } catch (IOException e) {
      Log.e(TAG, "IOException during reading script", e);
      throw new Exception("Failed to read script file", e);
    }
    return script.toString();
  }

  /**
   * Get response log buffer string
   * <br/>
   * @param void
   *
   * @return String response log buffer.
   */
  public String getRespOutLog() {
    return mRespOutlogBuilder.toString();
  }
}
