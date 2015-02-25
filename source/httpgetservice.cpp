#include "httpgetservice.h"

#include "debug.h"

u32 const kUnknownRequestSize = 0;
u32 const kUnknownHttpStatus = 0;
u32 const kDontWait = 0;

const u32 kHttpBufferSize = 32768;

HttpGetRequestState& request_error(HttpGetRequestState& state, HttpGetRequestError error) {
  switch (error) {
    case HttpGetRequestError::kCouldntAllocateContext:
      debug_message("Couldn't allocate context!");
      break;
    case HttpGetRequestError::kCouldntBeginReqest:
      debug_message("Couldn't begin request!");
      break;
    case HttpGetRequestError::kBadStatusCode:
      debug_message("Bad Status Code!");
      break;
    case HttpGetRequestError::kBadRequestState:
      debug_message("Bad Request State!");
      break;
    case HttpGetRequestError::kBadDownloadSize:
      debug_message("Bad Download Size!");
      break;
    case HttpGetRequestError::kReceiveDataFailed:
      debug_message("Receive Data Failed!");
      break;
    case HttpGetRequestError::kNone:
      break;
  }
  state.error = error;
  state.finished = true;

  // Try to close our open http context. (Don't panic if that fails either.)
  // This is to avoid leaking contexts in the case of an error, which is bound
  // to occur from time to time just due to network shenanigans  
  httpcCloseContext(&state.context);

  return state;
}

HttpGetRequestState InitiateRequest(std::string const& url,
    std::function<void(u32, u8 const*)> const on_data,
    u32 const expected_size) {
  HttpGetRequestState state;
  state.url = url;
  state.on_data = on_data;
  state.response.expected_size = expected_size;

  // Prepare the http:c service for this request

  Result ret = httpcOpenContext(&state.context, const_cast<char*>(url.c_str()),
    0);
  if (ret) {
    state.error = HttpGetRequestError::kCouldntBeginReqest;
    state.finished = true;
    debug_message("Couldn't allocate context!");
    return state;
  }

  ret = httpcBeginRequest(&state.context);
  if (ret) {
    return request_error(state, HttpGetRequestError::kCouldntBeginReqest);
    return state;
  }

  return state;
}

u8 g_http_buffer[kHttpBufferSize];

HttpGetRequestState ProcessRequest(HttpGetRequestState const& old_state) {
  Result ret;

  HttpGetRequestState state = old_state;
  if (state.finished or state.error != HttpGetRequestError::kNone) {
    // Do nothing! It is no longer safe to call any service functions.
    return state;
  }

  // TODO: Handle a global timeout for the entire request, and error properly
  // if this is taking way too long.

  // If the response hasn't arrived yet, bail for now; networks are slow, and we
  // can't afford to wait on them.
  httpcReqStatus status;
  ret = httpcGetRequestState(&state.context, &status);
  if (ret) {
    return request_error(state, HttpGetRequestError::kBadRequestState);
  }
  if (status == httpcReqStatus::HTTPCREQSTAT_INPROGRESS_REQSENT) {
    debug_message("Not here yet..");
    return state;
  }

  // If we still don't have a status code, attempt to grab it.
  if (state.status_code_ == kUnknownHttpStatus) {
    Result ret = httpcGetResponseStatusCode(&state.context, &state.status_code_,
        kDontWait);
    if (ret) {
      debug_message("Failed to retrieve status code!");
      return request_error(state, HttpGetRequestError::kBadStatusCode);
    }

    // If we received any status code other than 200, bail.
    if (state.status_code_ != 200) {
      debug_message("Status code: " + string_from<u32>(state.status_code_));
      return request_error(state, HttpGetRequestError::kBadStatusCode);
    }

    // Update the expected size of the download here (for reporting)
    ret = httpcGetDownloadSizeState(&state.context, NULL,
        &state.response.expected_size);
    if (ret) {
      return request_error(state, HttpGetRequestError::kBadDownloadSize);
    }
    debug_message("Got expected size: " + string_from<u32>(state.response.expected_size));
  }

  // Request data, up to the size of the buffer
  ret = httpcReceiveData(&state.context, g_http_buffer, kHttpBufferSize);
  if (ret == 0 or (u32)ret == HTTPC_RESULTCODE_DOWNLOADPENDING) {
    u32 old_size = state.response.current_size;
    ret = httpcGetDownloadSizeState(&state.context, &state.response.current_size, NULL);
    u32 transferred_this_round = state.response.current_size - old_size;

    // If we received any data this time, go ahead and output it
    if (transferred_this_round > 0) {
      state.on_data(transferred_this_round, g_http_buffer);
    }

    // If we've download all the data, we're done! Close the context and stop.
    if (state.response.current_size == state.response.expected_size) {
      httpcCloseContext(&state.context);
      state.finished = true;
    }
  } else {
    // Something bad happened! Connection closed maybe?
    debug_message("ret: " + string_from<u32>(ret));
    return request_error(state, HttpGetRequestError::kReceiveDataFailed);
  }

  // Finally, return this state object for tracking the status of the download
  return state;
}