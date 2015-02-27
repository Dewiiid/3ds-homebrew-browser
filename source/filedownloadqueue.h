#ifndef HOMEBREW_BROWSER_FILEDOWNLOADQUEUE_H_
#define HOMEBREW_BROWSER_FILEDOWNLOADQUEUE_H_

#include <vector>
#include <string>
#include <stdio.h>

#include "httpgetservice.h"



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

struct FileDownloadQueueState {
  std::vector<UrlPathLink> file_queue;

  bool finished{false};
  FileDownloadQueueError error{FileDownloadQueueError::kNone};
  
  HttpGetRequestState download_state_;
  FilePhase current_file_phase_{FilePhase::kNotStarted};
  //Handle current_file_handle_;
  FILE* current_file_handle_;
};

FileDownloadQueueState CreateQueue(std::vector<UrlPathLink> download_list);
FileDownloadQueueState ProcessQueue(FileDownloadQueueState const& state);

#endif
