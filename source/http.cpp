#include "http.h"

#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <map>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include "storage.h"
#include "util.h"
#include "debug.h"

using std::string;
using std::tuple;

namespace hbb = homebrew_browser;

struct url_components {
  string protocol;
  string hostname;
  string server;
  int port;
  string resource;
};

std::tuple<Result, std::vector<std::string>> hbb::download_and_split_on_newlines(std::string const& url) {
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

std::tuple<Result, std::vector<std::string>> hbb::get_file_listing_for_title(std::string const& server_url, std::string const& title) {
  return download_and_split_on_newlines(server_url + "/3ds/" + title + "/file_list");
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

// note: expects a well formed URL. Mal-formed URLs are likely to cause
// undefined behavior.
/*
url_components parse_url(string url) {
  url_components result;

  result.protocol = url.substr(0, url.find("://"));
  url = url.substr(result.protocol.size() + 3);

  result.hostname = url.substr(0, url.find("/"));
  url = url.substr(result.hostname.size() + 1);

  //attempt to parse out a port number, if it exists
  result.port = 80;
  if (result.hostname.find(":") < result.hostname.npos) {
    string str_port = result.hostname.substr(result.hostname.find(":") + 1);
    std::istringstream(str_port) >> result.port;
    result.hostname = result.hostname.substr(0, result.hostname.find(":"));
  }
  //attempt a hostname lookup, to resolve domain names
  result.server = hostname_to_ip(result.hostname);

  //finally, what remains is the resource itself
  result.resource = "/" + url;
  return result;
}*/

tuple<Result, std::vector<u8>> hbb::http_get(string const& url) {
  //url_components details = parse_url(url);

  /*

  // grab the header first
  int socket_desc = start_http_request(details);
  std::map<string, string> header = read_http_header(socket_desc);

  // using the header details, figure out the content length and read that
  // into a buffer
  if (header.count("Content-Length") > 0) {
    int content_length;
    std::istringstream(header["Content-Length"]) >> content_length;
    int bytes_read = read_http_content_into_buffer(socket_desc, content_length);
    if (bytes_read != content_length) {
      debug_message("Content length mismatch! Possible error!");
    }
    close_connection(socket_desc);
    return std::make_tuple(0, std::vector<u8>(g_content_buffer, g_content_buffer + g_content_length));
  } else {
    close_connection(socket_desc);
    debug_message("Content-Length not found in response! Bailing...");
    return std::make_tuple(-1, std::vector<u8>{});
  }

  close_connection(socket_desc);
  return std::make_tuple(-1, std::vector<u8>{});

  /*/


  //*
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

  /*
  std::string str_data(buffer.begin(), buffer.end());
  debug_message("HTTP URL: " + url);
  debug_message("HTTP GET: " + str_data);*/

  return std::make_tuple(ret, buffer);
  //*/
}

Result hbb::download_to_file(std::string const& url, std::string const& absolute_path, std::function<void (int)> report) {
  Result ret;
  std::vector<u8> data;
  report(0);

  std::tie(ret, data) = http_get(url);
  report(50);

  write_file(absolute_path, &data[0], data.size());
  report(100);
  
  return ret;
}