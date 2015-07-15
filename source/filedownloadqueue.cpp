#include "filedownloadqueue.h"
//#include "storage.h"

#include "debug.h"
#include "storage.h"
#include "util.h"

using namespace std;

namespace hbb = homebrew_browser;

hbb::FileDownloadQueueProgress hbb::CurrentProgress(FileDownloadQueueState const& state) {
  string current_filename = state.download_state_.url.substr(
      state.download_state_.url.rfind("/") + 1);
  int current_file = state.starting_file_count - state.file_queue.size() + 1;
  float progress_percent = state.download_state_.response.current_size * 100 /
          state.download_state_.response.expected_size;

  return {current_filename, current_file, state.starting_file_count,
      progress_percent};
}

hbb::FileDownloadQueueState hbb::CreateQueue(std::vector<UrlPathLink> download_list) {
  debug_message("Created a file queue!");
  FileDownloadQueueState state;
  state.file_queue = download_list;
  state.starting_file_count = download_list.size();
  return state;
}

hbb::FileDownloadQueueState hbb::ProcessQueue(FileDownloadQueueState const& old_state) {
  // If we're done, exit immediately. (Don't waste time.)
  if (old_state.finished or old_state.error != FileDownloadQueueError::kNone) {
    return old_state;
  }

  FileDownloadQueueState state = old_state;

  if (state.current_file_phase_ == FilePhase::kNotStarted and 
      state.file_queue.size() > 0) {
    debug_message("Starting a new file");

    // Open the file for reading
    //std::tie(ret, state.current_file_handle_) = open_file_for_writing(
    //    state.file_queue[0].path);
    string containing_directory = state.file_queue[0].path.substr(0, state.file_queue[0].path.rfind("/"));
    mkdirp(containing_directory);
    state.current_file_handle_ = fopen(state.file_queue[0].path.c_str(), "w");
    if (state.current_file_handle_ == nullptr) {
        state.error = FileDownloadQueueError::kFileOpenFailed;
        state.finished = true;
        return state;
    }
    debug_message("Opened " + state.file_queue[0].path + " for writing");

    // Initialize the HttpGetRequest for this file
    state.download_state_ = InitiateRequest(state.file_queue[0].url, 
        [=](u32 size, u8 const* data) -> Result {
          u32 bytes_written = fwrite(data, 1, size, state.current_file_handle_);
          if (bytes_written != size) {
            return -1;
          }
          //write_error = FSFILE_Write(state.current_file_handle_, nullptr, 0,
          //    data, size, FS_WRITE_FLUSH);
          return 0;
        });

    debug_message("Setting mode to Active");
    state.current_file_phase_ = FilePhase::kActive;
  }

  if (state.current_file_phase_ == FilePhase::kActive) {
    // Process the current download
    state.download_state_ = ProcessRequest(state.download_state_);  

    // If the current download ever generates an error, BAIL. The whole queue.
    // Bad things have happened. (TODO later: tolerate errors and requeue?
    // That's way too complicated right now.)
    if (state.download_state_.error != HttpGetRequestError::kNone) {
      debug_message("Download failed!");
      state.error = FileDownloadQueueError::kDownloadFailed;
      state.finished = true;
      // FSFILE_Close(state.current_file_handle_);
      fclose(state.current_file_handle_);
      return state;
    }

    // If the HttpGetRequest is finished, yay! Denote it as such
    if (state.download_state_.finished) {
      debug_message("File finished!");
      state.current_file_phase_ = FilePhase::kFinished;
    }

  }  

  // If the current download is finished, cleanup and move on to the next
  // file (if any)
  if (state.current_file_phase_ == FilePhase::kFinished) {
    debug_message("Cleaning up, preparing next file..");
    // Close the active file handle
    // FSFILE_Close(state.current_file_handle_);
    fclose(state.current_file_handle_);

    // Remove this file from the list
    state.file_queue.erase(state.file_queue.begin());

    if (state.file_queue.size() > 0) {
      // Prepare for the next file
      state.current_file_phase_ = FilePhase::kNotStarted;
    } else {
      // We're finally done!
      debug_message("Queue finished successfully!");
      state.finished = true;
    }
  }

  return state;
}
