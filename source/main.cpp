#include <stdlib.h>
#include <string.h>
#include <iso646.h>
#include <malloc.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <map>

#include <3ds.h>

#include "drawing.h"
#include "font.h"
#include "http.h"
#include "smdh.h"
#include "storage.h"
#include "ui.h"
#include "util.h"
#include "input.h"
#include "browser.h"
#include "debug.h"

#include "title_screen_bin.h"

using std::string;
using std::tuple;

std::array<ListingMetadata, 3> get_title_list_draw_state(
    FilteredListCursor const& cursor,
    std::array<AppInfo, 3> const& current_page_app_info) {
  bool const there_are_no_titles = cursor.begin == cursor.end;
  ListingMetadata const hidden{ListingTitleDisplay::kHidden, nullptr, "", ""};
  if (there_are_no_titles) {
    return {{hidden, hidden, hidden}};
  }

  auto const visible_titles_begin = std::next(cursor.begin,
      std::distance(cursor.begin, cursor.selected) / 3 * 3);
  auto const visible_titles_end =
      std::distance(visible_titles_begin, cursor.end) < 3 ?
      cursor.end : std::next(visible_titles_begin, 3);
  std::array<ListingMetadata, 3> visible_titles{{hidden, hidden, hidden}};
  std::transform(visible_titles_begin, visible_titles_end,
      begin(visible_titles), [](Title const* title) {
    return ListingMetadata{ListingTitleDisplay::kVisible, nullptr, title->title_name, ""};
  });

  // Overlay any information we have from the app info for each title.
  for (u32 app = 0; app < visible_titles.size(); ++app) {
    auto& title = visible_titles[app];
    auto const& app_info = current_page_app_info[app];

    title.icon = &(*app_info.image)[0];
    if (not app_info.title.empty()) {
      title.title = app_info.title;
    }
    // TODO: Add author to ListingMetadata, so it can be assigned here.
    title.description = app_info.description;
  }

  return visible_titles;
}

ListingScrollbar get_scrollbar_draw_state(FilteredListCursor const& cursor) {
  auto const list_size = std::distance(cursor.begin, cursor.end);
  auto const cursor_position = std::distance(cursor.begin, cursor.selected);
  if (list_size < 3) {
    return {ListingScrollbarDisplay::kHidden, 0};
  }
  return {
      ListingScrollbarDisplay::kVisible,
      100 * cursor_position / (list_size - 1)
    };
}

void initialize_sockets() {
  u32 ret = SOC_Initialize((u32*)memalign(0x1000, 0x100000), 0x100000);
  debug_message(string_from<unsigned int>(ret));
}

void fade_to_black() {
  for (int i = 0; i < 16; i++) {
    gspWaitForVBlank();
    for (int j = 0; j < 2; j++) {
      // fade top screen to black
      u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
      for (u8* color = fb; color < fb + 400 * 240 * 3; color++) {
        if (*color > 16) {
          *color = *color - 16;
        } else {
          *color = 0;
        }
      }
      // fade bottom screen to black
      fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
      for (u8* color = fb; color < fb + 320 * 240 * 3; color++) {
        if (*color > 16) {
          *color = *color - 16;
        } else {
          *color = 0;
        }
      }

      gfxFlushBuffers();
      gfxSwapBuffers();
    }
  }
}

int main()
{
  // Initialize services
  srvInit();
  aptInit();
  hidInit(NULL);
  gfxInit();  
  fsInit();
  httpcInit();
  initialize_storage();
  initialize_sockets();

  // throw our title onscreen (todo: make this part of UI maybe? It's
  // totally static for now)
  /*
  u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
  draw_sprite(title_screen_bin, fb, 0, 0);
  gfxFlushBuffers();
  gfxSwapBuffers();
  fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
  draw_sprite(title_screen_bin, fb, 0, 0);
  */

  consoleInit(GFX_TOP, nullptr);
  debug_message("Downloading homebrew list...");

  BrowserState state;
  Result error{0};
  std::tie(error, state.full_homebrew_list) = get_homebrew_listing(kServer);

  debug_message("Downloaded " + string_from<int>(state.full_homebrew_list.size()) + " titles!");

  // Main loop
  while (aptMainLoop())
  {
    gspWaitForVBlank();
    hidScanInput();
    u32 keys_down = hidKeysDown();
    touchPosition touch_position;
    hidTouchRead(&touch_position);
    
    u32 const old_selected_index = state.selected_index;
    handle_input(keys_down, touch_position, state);

    if (state.filtered_list_dirty) {
      sort_homebrew_list(state);
      filter_homebrew_list(state);
      download_smdh_for_page(kServer, get_title_list_cursor(state.filtered_homebrew_list,
          state.selected_index), state.app_info_for_current_page);
      state.filtered_list_dirty = false;
    }

    if (old_selected_index / 3 != state.selected_index / 3) {
      download_smdh_for_page(kServer, 
          get_title_list_cursor(state.filtered_homebrew_list, state.selected_index), 
          state.app_info_for_current_page);
    }

    draw_full_ui_from_state(ListingDrawState{
      state.selected_category,
      get_title_list_draw_state(
          get_title_list_cursor(state.filtered_homebrew_list, state.selected_index), 
          state.app_info_for_current_page),
      state.selected_index % 3,
      get_scrollbar_draw_state(
          get_title_list_cursor(state.filtered_homebrew_list, state.selected_index)),
      state.sort_order
    });

    // Flush and swap framebuffers
    gfxFlushBuffers();
    gfxSwapBuffers();
  }
  fade_to_black();

  // Exit services
  SOC_Shutdown();
  httpcExit();
  fsExit();
  gfxExit();
  hidExit();
  aptExit();
  srvExit();
  return 0;
}
