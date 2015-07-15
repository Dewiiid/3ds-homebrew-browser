#ifndef HOMEBREW_BROWSER_FILEDOWNLOADQUEUE_H_
#define HOMEBREW_BROWSER_FILEDOWNLOADQUEUE_H_

#include <vector>
#include <string>
#include <stdio.h>

#include "httpgetservice.h"

namespace homebrew_browser {

struct UrlPathLink {
  std::string url;
  std::string path;
};

enum class FileDownloadQueueError {
  kNone,
  kDownloadFailed,
  kFileOpenFailed,
  kFileWriteFailed,
};

enum class FilePhase {
  kNotStarted,
  kActive,
  kFinished
};

struct FileDownloadQueueProgress {
  std::string url;
  int current_file;
  int total_files;
  float progress_percent;
};

struct FileDownloadQueueState {
  std::vector<UrlPathLink> file_queue;

  bool finished{false};
  FileDownloadQueueError error{FileDownloadQueueError::kNone};
  
  HttpGetRequestState download_state_;
  FilePhase current_file_phase_{FilePhase::kNotStarted};
  //Handle current_file_handle_;
  FILE* current_file_handle_;
  int starting_file_count;
};

FileDownloadQueueState CreateQueue(std::vector<UrlPathLink> download_list);
FileDownloadQueueState ProcessQueue(FileDownloadQueueState const& state);
FileDownloadQueueProgress CurrentProgress(FileDownloadQueueState const& state);

}  // namespace homebrew_browser

#endif
