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

#include "drawing.h"
#include "http.h"
#include "storage.h"
#include "util.h"

using std::string;
using std::tuple;

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

  initialize_storage();

  string const kServer = "http://192.168.0.3:1337";

  Result error{0};
  std::vector<std::string> homebrew_listing;
  std::tie(error, homebrew_listing) = get_homebrew_listing(kServer);

  auto const& first_listing = homebrew_listing[0];

  std::vector<std::string> title_file_listing;
  std::tie(error, title_file_listing) = get_file_listing_for_title(kServer, first_listing);

  for (auto const& absolute_path : title_file_listing) {
    std::vector<u8> file_contents;
    std::tie(error, file_contents) = http_get(kServer + absolute_path);
    write_file(absolute_path, &file_contents[0], file_contents.size());
  }

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
