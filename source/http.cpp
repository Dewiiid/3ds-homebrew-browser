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

const int kMaxHeaderSize = 0x10000;
char g_response_header[kMaxHeaderSize];
int g_header_length;

const int kContentBufferSize = 2 * 1024 * 1024;  // 2 MB
u8 g_content_buffer[kContentBufferSize];
int g_content_length;

struct url_components {
  string protocol;
  string hostname;
  string server;
  int port;
  string resource;
};

string hostname_to_ip(string hostname) {
  hostent* he;
  he = gethostbyname(hostname.c_str());
  if (he == nullptr) {
    debug_message("DNS Lookup Failed: " + hostname);
    return hostname;
  }

  if (he->h_addr_list[0] != nullptr) {
    in_addr** host_address_list = reinterpret_cast<in_addr**>(he->h_addr_list);
    return string(inet_ntoa(*host_address_list[0]));
  }

  debug_message("Host not found: " + hostname);
  return hostname;
}

int init_connection(string ip_address, u16 port) {
  sockaddr_in server;
  server.sin_addr.s_addr = inet_addr(ip_address.c_str());
  server.sin_port = htons(port);
  server.sin_family = AF_INET;
  int socket_desc;

  //debug_message("Server: " + ip_address + string_from<unsigned int>(server.sin_addr.s_addr));

  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1) {
    debug_message("Could not create socket!");
    debug_message("Error code: " + string_from<signed int>(errno));
    return -1;
  }

  int error = connect(socket_desc, (sockaddr*)&server, sizeof(server));
  if (error < 0) {
    debug_message("Connection failed!");
    debug_message("Error code: " + string_from<signed int>(errno));
    return -1;
  }

  return socket_desc;
}

void close_connection(int socket_desc) {
  closesocket(socket_desc);
}

//Returns a socket descriptor on success, or -1 on failure.
int start_http_request(url_components request) {
  //open up a socket
  int socket_desc = init_connection(request.server, request.port);
  if (socket_desc < 0) {
    debug_message("Error initializing connection: " + string_from<unsigned int>(errno));
    return -1;
  }

  //craft a GET request
  string message = "GET " + request.resource + " HTTP/1.0\r\nAccept: application/homebrew.browser-v0.1+text\r\nHost: " + request.hostname + "\r\n\r\n";
  debug_message(message);
  int error = send(socket_desc, message.c_str(), message.size(), 0);
  if (error < 0) {
    debug_message("Error sending request!");
    debug_message("Error code: " + string_from<signed int>(errno));
    return -1;
  }

  return socket_desc;
}

bool buffer_contains_http_terminator(char* buffer) {
  return strstr(buffer, "\r\n\r\n") != nullptr;
}

//note: this uses the global buffer
std::map<string, string> parse_http_header(string raw_header) {
  std::map<string, string> header_pairs;
  raw_header = raw_header.substr(0, raw_header.find("\r\n\r\n") + 2);
  //return header_pairs;

  // read off the first line; it's the HTTP status code, and is not
  // necessary at this time
  // TODO: maybe bail if the status code isn't 200?
  if (raw_header.find("\r\n") != string::npos) {
    string status_code = raw_header.substr(0, raw_header.find("\r\n"));
    debug_message("status: " + status_code);
    raw_header = raw_header.substr(status_code.size() + 2);
    string protocol;
    int response_code;
    string response_name;
    std::istringstream(status_code) >>
        protocol >> response_code >> response_name;
    if (response_code != 200) {
      debug_message("Response " + string_from<int>(response_code) + ": "
          + response_name);
      return header_pairs;
    }
  } else {
    debug_message("What is this noise?");
    return header_pairs;
  }

  //debug_message("HEADER without STATUS: ");
  //debug_message(raw_header);

  while (raw_header.find("\r\n") != string::npos) {
    // extract one line exactly
    string line = raw_header.substr(0, raw_header.find("\r\n"));
    //debug_message("LINE: " + line, true);
    // remove lines from the header as we go
    raw_header = raw_header.substr(line.size() + 2);

    //debug_message("HEADER AFTER LINE:");
    //debug_message(raw_header);

    //return header_pairs;

    if (line.find(": ") != string::npos) {
      string key = line.substr(0, line.find(": "));
      string value = line.substr(key.size() + 2);
      header_pairs[key] = value;
      //debug_message("HTTP Header:");
      debug_message(key + " --> " + value);
    }
  }
  return header_pairs;
}

std::map<string, string> read_http_header(int socket_desc) {
  //NULL out the current buffer (this also allows string functions to work)
  g_header_length = 0;
  memset(g_response_header, 0, kMaxHeaderSize);

  while(not buffer_contains_http_terminator(g_response_header) and g_header_length <= kMaxHeaderSize) {
    // attempt to read data from the socket into the END of the buffer,
    // with a limit in place to make sure we don't overflow it
    int bytes_read = recv(socket_desc, g_response_header + g_header_length, 
        kMaxHeaderSize - g_header_length, 0);
    //debug_message("Read " + string_from<int>(bytes_read) + " bytes in header!");
    if (bytes_read < 0) {
      debug_message("Error reading response header!");
      debug_message("Error code: " + string_from<signed int>(errno));
      return std::map<string, string>();
    }
    g_header_length += bytes_read;
  }

  return parse_http_header(string(g_response_header, g_header_length));
}

int read_http_content_into_file(int socket_desc, u32 content_length, 
    std::string const& absolute_path, std::function<void (int)> report) {
  //Open up the file, make sure that works!
  Result error;
  Handle file_handle;
  std::tie(error, file_handle) = open_file_for_writing(absolute_path);
  if (error) {
    return 0;  // bytes read was 0; we failed
  }

  u32 bytes_written = 0;
  u32 content_received = 0;

  // first things first, there might already be some content after the http
  // header, so let's get that copied away
  char* header_content_start = strstr(g_response_header, "\r\n\r\n") + 4;
  int header_content_length = g_header_length - (header_content_start - g_response_header);
  FSFILE_Write(file_handle, &bytes_written, content_received, header_content_start, header_content_length, FS_WRITE_FLUSH);
  content_received += header_content_length;

  u32 last_report = 0;
  u32 successive_failed_reads = 0;
  while (content_received < content_length) {
    int bytes_read = recv(socket_desc, g_content_buffer,
        kContentBufferSize, 0);
    if (bytes_read < 0) {
      debug_message("Error reading response content!");
      debug_message("Error code: " + string_from<signed int>(errno));
      //close the file handle first!
      FSFILE_Close(file_handle);
      return -1;
    }
    //write this content out to file
    FSFILE_Write(file_handle, &bytes_written, content_received, g_content_buffer, bytes_read, FS_WRITE_FLUSH);
    content_received += bytes_read;
    //debug_message(string_from<int>(content_received) + "/" + string_from<int>(content_length) + " " + absolute_path);
    if (bytes_read == 0 or content_received > last_report + 4096) {
     report(content_received * 100 / content_length);
    }

    if (bytes_read == 0) {
      successive_failed_reads++;
    } else {
      successive_failed_reads = 0;
    }

    if (successive_failed_reads > 1800) {  // 1800 is roughly 30 seconds worth of frames, this is a sensible timeout
      debug_message("Download failed! Aborting.");
      break;
    }
  }
  FSFILE_Close(file_handle);
  return content_received;
}

int read_http_content_into_buffer(int socket_desc, int content_length) {
  //clear out the content buffer
  memset(g_content_buffer, 0, kContentBufferSize);
  g_content_length = 0;

  // first things first, there might already be some content after the http
  // header, so let's get that copied away
  char* header_content_start = strstr(g_response_header, "\r\n\r\n") + 4;
  int header_content_length = g_header_length - (header_content_start - g_response_header);
  memcpy(g_content_buffer, header_content_start, header_content_length);
  g_content_length += header_content_length;

  // now, loop over recv() and grab any remaining content, up to the content
  // length.
  while (g_content_length < content_length) {
    int bytes_read = recv(socket_desc, g_content_buffer + g_content_length,
        kContentBufferSize - g_content_length, 0);
    if (bytes_read < 0) {
      debug_message("Error reading response content!");
      debug_message("Error code: " + string_from<signed int>(errno));
      return -1;
    }

    g_content_length += bytes_read;
  }

  return content_length;
}

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

// note: expects a well formed URL. Mal-formed URLs are likely to cause
// undefined behavior.
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
}

tuple<Result, std::vector<u8>> http_get(string const& url) {
  url_components details = parse_url(url);

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


  /*
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
  */
}

Result download_to_file(std::string const& url, std::string const& absolute_path,
    std::function<void (int)> report) {
  url_components details = parse_url(url);

  // grab the header first
  int socket_desc = start_http_request(details);
  std::map<string, string> header = read_http_header(socket_desc);

  if (header.count("Content-Length") > 0) {
    int content_length;
    std::istringstream(header["Content-Length"]) >> content_length;
    int bytes_read = read_http_content_into_file(socket_desc, content_length, absolute_path, report);
    if (bytes_read != content_length) {
      debug_message("Content length mismatch! Possible error!");
    }
    close_connection(socket_desc);
  }  else {
    close_connection(socket_desc);
    debug_message("Content-Length not found in response! Bailing...");
    return -1;
  }

  //Success?
  close_connection(socket_desc);
  return 0;
}