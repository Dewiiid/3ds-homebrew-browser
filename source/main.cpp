#include <stdlib.h>
#include <string.h>
#include <iso646.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <3ds.h>

#include "category_emulators_normal_bin.h"
#include "category_emulators_selected_bin.h"
#include "category_games_normal_bin.h"
#include "category_games_selected_bin.h"
#include "category_media_normal_bin.h"
#include "category_media_selected_bin.h"
#include "category_misc_normal_bin.h"
#include "category_misc_selected_bin.h"
#include "category_tools_normal_bin.h"
#include "category_tools_selected_bin.h"
#include "row_base_bin.h"
#include "scrollbar_bin.h"
#include "sort_normal_bin.h"
#include "sort_reversed_bin.h"
#include "ui_bar_bin.h"

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

void blit_column(u8 const* source, u8* dest, u32 pixel_count) {
  memcpy(dest, source, 3 * pixel_count);
}

void blit(u8 const* source, u8* dest, u32 rows, u32 columns) {
  for (u32 i = 0; i < columns; ++i) {
    blit_column(source + 3 * rows * i, dest + 3 * 240 * i, rows);
  }
}

void blit_sprite(u8 const* source, u8* dest) {
  u32 width = *reinterpret_cast<u32 const*>(source);
  u32 height = *reinterpret_cast<u32 const*>(source + 4);
  blit(source + 8, dest, height, width);
}

void draw_sprite(u8 const* source, u8* framebuffer, u32 x, u32 y) {
  u32 height = *reinterpret_cast<u32 const*>(source + 4);
  // Convert from typical screen coordinates to framebuffer coordinates.
  blit_sprite(source, framebuffer + 3 * (240 - y - height + x * 240));
}

void draw_solid_background(u8* framebuffer, u32 pixel_count, u8 r, u8 g, u8 b) {
  for (u32 i = 0; i < pixel_count; ++i) {
    framebuffer[3 * i + 0] = b;
    framebuffer[3 * i + 1] = g;
    framebuffer[3 * i + 2] = r;
  }
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
    // u8 image[]{
    //   0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    //   0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF
    // };
    u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
    // memset(fb, 0, 240*320*3);
    draw_solid_background(fb, 240 * 320, 216, 201, 201);
    // blit_column(image, fb + 3 * (50 + 10 * 240), 4);
    // blit(image, fb + 3 * (50 + 10 * 240), 2, 2);
    // blit(category_games_normal_bin, fb + 3 * (50 + 10 * 240), 37, 43);
    draw_sprite(category_games_normal_bin, fb, 4, 6);
    draw_sprite(category_media_normal_bin, fb, 4, 48);
    draw_sprite(category_emulators_normal_bin, fb, 4, 90);
    draw_sprite(category_tools_normal_bin, fb, 4, 132);
    draw_sprite(category_misc_normal_bin, fb, 4, 174);

    draw_sprite(row_base_bin, fb, 51, 3);
    draw_sprite(row_base_bin, fb, 51, 74);
    draw_sprite(row_base_bin, fb, 51, 145);

    draw_sprite(ui_bar_bin, fb, 0, 216);
    draw_sprite(sort_reversed_bin, fb, 265, 217);

    draw_sprite(scrollbar_bin, fb, 304, 3);
    // fb[3*(50+10*240)] = 0xFF;
    // fb[3*(50+10*240)+1] = 0xFF;
    // fb[3*(50+10*240)+2] = 0xFF;

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
