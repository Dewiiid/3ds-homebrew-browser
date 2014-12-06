#include <stdlib.h>
#include <string.h>
#include <iso646.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <3ds.h>

using std::string;
using std::tuple;

void setup_frame_buffer(u8 lightness) {
  u8 *lower_frame_buffer = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
  memset(lower_frame_buffer, lightness, 240 * 320 * 3);
  gfxFlushBuffers();
  gfxSwapBuffers();

  lower_frame_buffer = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
  memset(lower_frame_buffer, lightness, 240 * 320 * 3);
  gfxFlushBuffers();
  gfxSwapBuffers();

  gspWaitForVBlank();
}

void copy_to_frame_buffer(u8* buffer, u32 byte_count) {
  u8 *upper_frame_buffer = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
  u8 *lower_frame_buffer = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
  memset(lower_frame_buffer, 0xff, 240 * 320 * 3);
  memcpy(upper_frame_buffer, buffer, byte_count);
  gfxFlushBuffers();
  gfxSwapBuffers();

  upper_frame_buffer = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
  lower_frame_buffer = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
  memset(lower_frame_buffer, 0xff, 240 * 320 * 3);
  memcpy(upper_frame_buffer, buffer, byte_count);
  gfxFlushBuffers();
  gfxSwapBuffers();

  gspWaitForVBlank();
}

Result http_download(httpcContext *context, u8 **buffer, u32 *size_bytes) {
  // setup_frame_buffer(0xFF);

  *buffer = NULL;
  *size_bytes = 0;

  Result ret = httpcBeginRequest(context);
  if (ret)
  {
    return ret;
  }


  u32 status_code = 0;
  ret = httpcGetResponseStatusCode(context, &status_code, 0);
  if (ret)
  {
    return ret;
  }
  if (status_code != 200)
  {
    return -2;
  }

  ret = httpcGetDownloadSizeState(context, NULL, size_bytes);
  if (ret)
  {
    return ret;
  }

  *buffer = (u8*)malloc(*size_bytes);
  if (not *buffer)
  {
    return -1;
  }
  memset(*buffer, 0, *size_bytes);

  // setup_frame_buffer(0x0);

  ret = httpcDownloadData(context, *buffer, *size_bytes, NULL);
  if (ret)
  {
    free(*buffer);
    return ret;
  }

  // u32 size = size_bytes;
  // if (240 * 400 * 3 < size)
  // {
  //   size = 240 * 400 * 3;
  // }

  // copy_to_frame_buffer(buffer, size);

  // free(buf);

  return 0;
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

Result http_get(char *url, u8 **buffer, u32 *bytes_received) {
  *buffer = NULL;
  *bytes_received = 0;

  httpcContext context;
  Result ret = httpcOpenContext(&context, url, 0);
  if (ret)
  {
    return ret;
  }

  ret = http_download(&context, buffer, bytes_received);
  if (ret)
  {
    return ret;
  }
  httpcCloseContext(&context);
  return 0;
}

tuple<Result, std::vector<u8>> http_get(string const& url) {
  httpcContext context;
  Result ret = httpcOpenContext(&context, const_cast<char*>(url.c_str()), 0);
  if (ret)
  {
    return std::make_tuple(ret, std::vector<u8>{});
  }

  std::vector<u8> buffer{};
  std::tie(ret, buffer) = http_download(context);
  if (ret)
  {
    return std::make_tuple(ret, std::vector<u8>{});
  }
  return std::make_tuple(ret, buffer);
}

FS_archive sdmc_archive;

u32 write_file(const char *path, const char *filename, void *data, u32 byte_count) {
  /*Result ret = */FSUSER_CreateDirectory(NULL, sdmc_archive, FS_makePath(PATH_CHAR, path));
  Handle file_handle;
  char absolute_filename[256] = {};
  strcpy(absolute_filename, path);
  strcat(absolute_filename, filename);
  /*ret = */FSUSER_OpenFile(NULL, &file_handle, sdmc_archive, FS_makePath(PATH_CHAR, absolute_filename), FS_OPEN_WRITE | FS_OPEN_CREATE, 0);
  u32 bytes_written = 0;
  u32 write_offset_bytes = 0;
  FSFILE_Write(file_handle, &bytes_written, write_offset_bytes, data, byte_count, FS_WRITE_FLUSH);
  FSFILE_Close(file_handle);
  return bytes_written;
}

int main()
{
  // Initialize services
  srvInit();
  aptInit();
  hidInit(NULL);
  gfxInit();
  //gfxSet3D(true); // uncomment if using stereoscopic 3D
  fsInit();
  httpcInit();

  sdmc_archive = (FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
  FSUSER_OpenArchive(NULL, &sdmc_archive);

  #define SERVER string("http://192.168.0.3:1337")

  std::vector<u8> raw_unterminated_homebrew_listing{};
  Result ret{0};
  std::tie(ret, raw_unterminated_homebrew_listing) = http_get(SERVER + "/homebrew_list");
  string listing{
      (char*)&raw_unterminated_homebrew_listing[0],
      (char*)&raw_unterminated_homebrew_listing[0] + raw_unterminated_homebrew_listing.size()};

  string first_listing{listing.substr(0, listing.find_first_of('\n'))};
  std::vector<u8> raw_file_listing{};
  std::tie(ret, raw_file_listing) = http_get(SERVER + "/" + first_listing + "/file_list");
  string file_listing{
      (char*)&raw_file_listing[0],
      (char*)&raw_file_listing[0] + raw_file_listing.size()};

  std::istringstream file_listing_splitter{file_listing};
  string absolute_path{};
  while (std::getline(file_listing_splitter, absolute_path)) {
    std::vector<u8> file_contents{};
    std::tie(ret, file_contents) = http_get(SERVER + absolute_path);

    size_t const position_of_last_slash = absolute_path.find_last_of('/');
    string const directory = absolute_path.substr(0, position_of_last_slash + 1);
    string const filename = absolute_path.substr(position_of_last_slash + 1);
    write_file(directory.c_str(), filename.c_str(), &file_contents[0], file_contents.size());
  }

  // char *absolute_path = strtok((char*)file_listing, "\n");
  // while (absolute_path) {
  //   strcpy(url_buffer, SERVER.c_str());
  //   strcat(url_buffer, absolute_path);
  //   u8 *file_contents = NULL;
  //   http_get(url_buffer, &file_contents, &bytes_received);

  //   char *last_directory_separator = strrchr(absolute_path, '/');
  //   char directory[256] = {};
  //   strncpy(directory, absolute_path, last_directory_separator - absolute_path + 1);
  //   char *filename = last_directory_separator + 1;
  //   write_file(directory, filename, file_contents, bytes_received);

  //   free(file_contents);
  //   absolute_path = strtok(NULL, "\n");
  // }
  // free(file_listing);

  setup_frame_buffer(0x40);

  // Main loop
  while (aptMainLoop())
  {
    gspWaitForVBlank();
    hidScanInput();

    // Your code goes here

    u32 kDown = hidKeysDown();
    if (kDown & KEY_START)
      break; // break in order to return to hbmenu

    // Example rendering code that displays a white pixel
    // Please note that the 3DS screens are sideways (thus 240x400 and 240x320)
    // u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    // memset(fb, 0, 240*400*3);
    // fb[3*(10+10*240)] = 0xFF;
    // fb[3*(10+10*240)+1] = 0xFF;
    // fb[3*(10+10*240)+2] = 0xFF;

    // Flush and swap framebuffers
    gfxFlushBuffers();
    gfxSwapBuffers();
  }

  // Exit services
  httpcExit();
  fsExit();
  gfxExit();
  hidExit();
  aptExit();
  srvExit();
  return 0;
}
