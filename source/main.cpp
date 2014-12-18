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

#include <3ds.h>

#include "drawing.h"
#include "font.h"
#include "http.h"
#include "smdh.h"
#include "storage.h"
#include "ui.h"
#include "util.h"
#include "debug.h"

using std::string;
using std::tuple;

using Title = std::string;
using TitleList = std::vector<Title>;
struct TitleListCursor {
  typename TitleList::const_iterator begin;
  typename TitleList::const_iterator end;
  typename TitleList::const_iterator selected;
};

struct AppInfo {
  std::array<u8, 48 * 48 * 3>* image;
  std::string title;  // An empty title denotes an invalid struct.
  std::string author;
  std::string description;
};

TitleListCursor get_title_list_cursor(TitleList const& titles,
    TitleList::size_type const& offset) {
  return TitleListCursor{
    begin(titles),
    end(titles),
    std::next(begin(titles), offset)
  };
}

std::array<ListingMetadata, 3> get_title_list_draw_state(
    TitleListCursor const& cursor,
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
      begin(visible_titles), [](Title const& title) {
    return ListingMetadata{ListingTitleDisplay::kVisible, nullptr, title, ""};
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

ListingScrollbar get_scrollbar_draw_state(TitleListCursor const& cursor) {
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

Result download_app(std::string const& server, std::string const& title) {
  debug_message("Downloading " + title + " from " + server + "...", true);
  Result error{0};
  std::vector<std::string> title_file_listing;
  std::tie(error, title_file_listing) =
      get_file_listing_for_title(server, title);

  for (auto const& absolute_path : title_file_listing) {
    debug_message("Downloading file: " + absolute_path, true);
    std::vector<u8> file_contents;
    std::tie(error, file_contents) = http_get(server + absolute_path);
    write_file(absolute_path, &file_contents[0], file_contents.size());
  }
  debug_message("Finished.");
  return error;
}

Result download_smdh(std::string const& server, Title const& title, AppInfo& app_info) {
  Result error{0};
  std::vector<u8> smdh_byte_buffer;
  std::tie(error, smdh_byte_buffer) = http_get(server + "/" + title + "/smdh");
  if (error) {
    return error;
  }

  char smdh_title[0x40];
  char smdh_author[0x40];
  char smdh_description[0x80];
  smdh_s* const smdh = reinterpret_cast<smdh_s*>(&smdh_byte_buffer[0]);

  extractSmdhData(smdh, smdh_title, smdh_description, smdh_author, &(*app_info.image)[0]);
  app_info.title = smdh_title;
  app_info.author = smdh_author;
  app_info.description = smdh_description;

  return error;
}

void download_smdh_for_page(std::string const& server,
    TitleListCursor const& cursor, std::array<AppInfo, 3>& smdh_cache) {
  bool const there_are_no_titles = cursor.begin == cursor.end;
  if (there_are_no_titles) {
    std::for_each(begin(smdh_cache), end(smdh_cache), [](AppInfo& info) {
      info.title = "no title";
      info.author = "";
      info.description = "";
    });
    return;
  }

  auto const visible_titles_begin = std::next(cursor.begin,
      std::distance(cursor.begin, cursor.selected) / 3 * 3);
  auto const visible_titles_end =
      std::distance(visible_titles_begin, cursor.end) < 3 ?
      cursor.end : std::next(visible_titles_begin, 3);

  // std::transform(visible_titles_begin, visible_titles_end, begin(smdh_cache),
  //     [&server](Title const& title) {
  auto app_info = begin(smdh_cache);
  auto visible_title = visible_titles_begin;
  for (; app_info != end(smdh_cache) and visible_title != visible_titles_end;
      ++app_info, ++visible_title) {
    // This assumes that there is always an icon. This wont always be the case,
    // so this needs to be adjusted to place a default icon in the slot and
    // empty out the strings on a failed download.
    Result error = download_smdh(server, *visible_title, *app_info);
    if (error) {
      debug_message("SMDH Download ERR: " + string_from<int>(error));
    }
  }
  // });
}

std::array<AppInfo, 3> app_info_for_current_page{{
  {new std::array<u8, 48 * 48 * 3>()},
  {new std::array<u8, 48 * 48 * 3>()},
  {new std::array<u8, 48 * 48 * 3>()}
}};

void initialize_sockets() {
  u32 ret = SOC_Initialize((u32*)memalign(0x1000, 0x100000), 0x100000);
  debug_message(string_from<unsigned int>(ret));
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

  initialize_storage();
  initialize_sockets();

  consoleInit(GFX_TOP, nullptr);

  debug_message("Test!");
  debug_message("Blargh!!!");
  debug_message("Pineapples!");
  

  //string const kServer = "http://192.168.0.3:1337";
  //string const kServer = "http://darknovagames.com:1337";
  string const kServer = "http://23.21.136.4:1337";

  //*
  Result error{0};
  TitleList homebrew_listing;
  std::tie(error, homebrew_listing) = get_homebrew_listing(kServer);

  u32 selected_index = 0;

  SelectedCategory selected_category = SelectedCategory::kNone;

  //*
  download_smdh_for_page(kServer, get_title_list_cursor(homebrew_listing,
      selected_index), app_info_for_current_page);
  /*/
  // app_info_for_current_page = std::array<AppInfo, 3>{{}};
  //*/
  // Main loop
  while (aptMainLoop())
  {
    gspWaitForVBlank();
    hidScanInput();

    // Your code goes here

    u32 kDown = hidKeysDown();
    if (kDown & KEY_START) {
        break;  // Break in order to return to Homebrew Launcher.
    }
    u32 const old_selected_index = selected_index;
    if (kDown & KEY_UP) {
      selected_index = selected_index ? selected_index - 1 : 0;
    }
    if (kDown & KEY_DOWN) {
      selected_index = selected_index < homebrew_listing.size() - 1 ? selected_index + 1 : homebrew_listing.size() - 1;
    }
    if (kDown & KEY_A) {
      download_app(kServer, homebrew_listing[selected_index]);
    }
    if (kDown & KEY_L and selected_category > SelectedCategory::kNone) {
      selected_category = static_cast<SelectedCategory>(static_cast<int>(selected_category) - 1);
    }
    if (kDown & KEY_R and selected_category < SelectedCategory::kMisc) {
      selected_category = static_cast<SelectedCategory>(static_cast<int>(selected_category) + 1);
    }

    if (old_selected_index / 3 != selected_index / 3) {
      download_smdh_for_page(kServer, get_title_list_cursor(homebrew_listing,
          selected_index), app_info_for_current_page);
    }

    draw_full_ui_from_state(ListingDrawState{
      selected_category,
      get_title_list_draw_state(get_title_list_cursor(homebrew_listing, selected_index), app_info_for_current_page),
      selected_index % 3,
      get_scrollbar_draw_state(get_title_list_cursor(homebrew_listing, selected_index)),
      ListingSortOrder::kAlphanumericDescending
    });

    // Flush and swap framebuffers
    gfxFlushBuffers();
    gfxSwapBuffers();
  }

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
