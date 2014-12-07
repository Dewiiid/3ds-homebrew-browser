#include <stdlib.h>
#include <string.h>
#include <iso646.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <3ds.h>

#include "drawing.h"
#include "http.h"
#include "storage.h"
#include "ui.h"
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

    // redraw_full_ui();
    draw_full_ui_from_state(ListingDrawState{
      SelectedCategory::kEmulators,
      {{
        {ListingTitleDisplay::kVisible, nullptr, "", ""},
        {ListingTitleDisplay::kVisible, nullptr, "", ""},
        {ListingTitleDisplay::kHidden, nullptr, "", ""}
      }},
      1,
      {ListingScrollbarDisplay::kHidden, 100},
      ListingSortOrder::kAlphanumericAscending
    });

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
