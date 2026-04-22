/******************************************************************************
 *
 *  Copyright 2025-2026 NXP
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
#include <SEUpdaterClient.h>
#include <android-base/logging.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <string_view>

#undef LOG_TAG
#define LOG_TAG "se_update_agent"

const char* G_LOCK_PATH = "/data/vendor/se_update_agent/process_sync.lock";

void printUsageAndExit() {
  LOG(ERROR) << "Invalid arguments provided.";
  LOG(ERROR) << "Usage: se_update_agent <option> <script-dir-path>";
  LOG(ERROR) << "Options:";

  const std::vector<std::string> options = {"check-update", "apply-update",
                                            "log-status", "retry-check-update",
                                            "execute"};

  for (const auto& opt : options) {
    LOG(ERROR) << "  - " << opt;
  }

  LOG(ERROR) << "Exiting se_update_agent.";
  exit(EXIT_FAILURE);
}

/**
 * Attempts to acquire an exclusive lock on a file.
 * Returns the file descriptor on success, or -1 on failure.
 */
int acquire_file_lock() {
  int fd = open(G_LOCK_PATH, O_RDWR | O_CREAT | O_CLOEXEC, 0644);
  if (fd == -1) {
    LOG(ERROR) << "Open failed for " << G_LOCK_PATH << ": " << strerror(errno);
    return -1;
  }

  if (flock(fd, LOCK_EX) == -1) {
    LOG(ERROR) << "flock failed: " << strerror(errno);
    close(fd);
    return -1;
  }

  return fd;
}

/**
 * Releases the lock and closes the file descriptor.
 */
void release_file_lock(int fd) {
  if (fd < 0) {
    return;
  }

  flock(fd, LOCK_UN);
  close(fd);
}

int main(int argc, char* argv[]) {
  try {
    LOG(INFO) << "se_update_agent starting up !!!";

    if (argc < 2 || argc > 3) {
      printUsageAndExit();
    }

    const std::string_view action{argv[1]};
    const std::string script_dir_path =
        (argc == 3) ? std::string{argv[2]} : std::string{};

    // Serialize execution via file-based locking.
    int lock_fd = acquire_file_lock();
    if (lock_fd == -1) {
      // This fails during early boot when /data is not yet mounted.
      LOG(WARNING)
          << "Lock acquisition failed; likely due to unmounted /data partition.";
    }

    if (!strcmp(argv[1], "check-update")) {
      LOG(INFO) << "Performing action check-update";
      PrepareUpdate(script_dir_path);

    } else if (!strcmp(argv[1], "apply-update")) {
      LOG(INFO) << "Performing action apply-update";
      PerformUpdate(script_dir_path);

    } else if (!strcmp(argv[1], "log-status")) {
      LOG(INFO) << "Performing action log-status";
      LogVersionInfo(script_dir_path);

    } else if (!strcmp(argv[1], "retry-check-update")) {
      LOG(INFO) << "Performing action retry-check-update";
      RetryPrepareUpdate(script_dir_path);

    } else if (!strcmp(argv[1], "execute")) {
      LOG(INFO) << "Performing action execute";
      RunSingleScriptNoVersionCheck();

    } else {
  #ifdef NXP_BOOTTIME_UPDATE
      LOG(INFO) << "perform Legacy Boottime update";
      checkEseClientUpdate();
      perform_eSEClientUpdate();
  #endif
    }

    release_file_lock(lock_fd);
    LOG(INFO) << "se_update_agent exiting";
    return 0;
  } catch (const std::exception& e) {
    LOG(ERROR) << "Fatal exception: " << e.what();
  } catch (...) {
    LOG(ERROR) << "Fatal unknown exception";
  }

  return EXIT_FAILURE;

}
