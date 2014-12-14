#include "http.h"

#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "util.h"
#include "debug.h"

using std::string;
using std::tuple;

std::tuple<Result, std::vector<std::string>> download_and_split_on_newlines(std::string const& url) {
  Result error{0};
  std::vector<u8> raw_response;
  std::tie(error, raw_response) = http_get(url);
  if (error)
  {
    return std::make_tuple(error, std::vector<std::string>{});
  }

  std::istringstream splitter{string_from_bytes(raw_response)};

  raw_response.clear();
  raw_response.shrink_to_fit();

  std::vector<std::string> lines;
  std::string line;
  while (std::getline(splitter, line)) {
    lines.push_back(line);
  }
  return std::make_tuple(error, lines);
}

std::tuple<Result, std::vector<std::string>> get_homebrew_listing(std::string const& server_url) {
  return download_and_split_on_newlines(server_url + "/homebrew_list");
}

std::tuple<Result, std::vector<std::string>> get_file_listing_for_title(std::string const& server_url, std::string const& title) {
  return download_and_split_on_newlines(server_url + "/" + title + "/file_list");
}

tuple<Result, std::vector<u8>> http_download(httpcContext& context) {
  std::vector<u8> buffer{};
  Result ret = httpcBeginRequest(&context);
  if (ret)
  {
    return std::make_tuple(ret, buffer);
  }

  u32 status_code = 0;
  ret = httpcGetResponseStatusCode(&context, &status_code, 0);
  if (ret)
  {
    return std::make_tuple(ret, buffer);
  }
  if (status_code != 200)
  {
    return std::make_tuple(-2, buffer);
  }

  u32 size_bytes = 0;
  ret = httpcGetDownloadSizeState(&context, NULL, &size_bytes);
  if (ret)
  {
    return std::make_tuple(ret, buffer);
  }

  buffer.resize(size_bytes, 0);
  ret = httpcDownloadData(&context, &buffer[0], buffer.size(), NULL);
  if (ret)
  {
    buffer.clear();
    buffer.shrink_to_fit();
    return std::make_tuple(ret, buffer);
  }

  return std::make_tuple(0, buffer);
}

tuple<Result, std::vector<u8>> http_get(string const& url) {
  httpcContext context;
  Result ret = httpcOpenContext(&context, const_cast<char*>(url.c_str()), 0);
  if (ret)
  {
    debug_message("Failed to open context, err: " + string_from<int>(ret));
    debug_message("Failing URL: " + url);
    return std::make_tuple(ret, std::vector<u8>{});
  }

  std::vector<u8> buffer{};
  std::tie(ret, buffer) = http_download(context);
  httpcCloseContext(&context);
  if (ret)
  {
    debug_message("Failed to download GET response, err: " + string_from<int>(ret));
    debug_message("Failing URL: " + url);
    return std::make_tuple(ret, std::vector<u8>{});
  }
  return std::make_tuple(ret, buffer);
}
