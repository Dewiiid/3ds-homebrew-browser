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
#include "debug.h"

using std::string;
using std::tuple;

struct Title {
  string path;
  string title_name;
  string category_name;

  bool operator<(const Title& other) {
    return title_name < other.title_name;
  }
};

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

struct BrowserState {
  u32 selected_index = 0;
  SelectedCategory selected_category = SelectedCategory::kNone;

  TitleList homebrew_listing;
  std::array<AppInfo, 3> app_info_for_current_page{{
    {new std::array<u8, 48 * 48 * 3>()},
    {new std::array<u8, 48 * 48 * 3>()},
    {new std::array<u8, 48 * 48 * 3>()}
  }};
  ListingSortOrder sort_order{ListingSortOrder::kAlphanumericDescending};
};

string const kServer = "http://23.21.136.4:1337";

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
    return ListingMetadata{ListingTitleDisplay::kVisible, nullptr, title.title_name, ""};
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
  string appname = title.substr(title.find("/") + 1);
  debug_message("Downloading " + title + " from " + server + "...", true);
  Result error{0};
  std::vector<std::string> title_file_listing;
  std::tie(error, title_file_listing) =
      get_file_listing_for_title(server, title);

  for (auto const& relative_path : title_file_listing) {
    string server_path = "/3ds/" + title + "/" + relative_path;
    debug_message("Downloading file: " + server_path, true);
    std::vector<u8> file_contents;
    std::tie(error, file_contents) = http_get(server + server_path);
    debug_message("Writing file: /3ds/" + appname + "/" + relative_path);
    write_file("/3ds/" + appname + "/" + relative_path, 
        &file_contents[0], file_contents.size());
  }
  debug_message("Finished.");
  return error;
}

Result download_smdh(std::string const& server, Title const& title, AppInfo& app_info) {
  Result error{0};
  std::vector<u8> smdh_byte_buffer;
  std::tie(error, smdh_byte_buffer) = http_get(server + "/" + title.path + "/smdh");
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

void initialize_sockets() {
  u32 ret = SOC_Initialize((u32*)memalign(0x1000, 0x100000), 0x100000);
  debug_message(string_from<unsigned int>(ret));
}

std::map<SelectedCategory, string> g_category_names {
  {SelectedCategory::kGames, "games"},
  {SelectedCategory::kMedia, "media"},
  {SelectedCategory::kEmulators, "emulators"},
  {SelectedCategory::kTools, "tools"},
  {SelectedCategory::kMisc, "misc"}
};


std::tuple<Result, std::vector<Title>> get_homebrew_listing(std::string const& server_url, SelectedCategory category) {
  Result error;
  std::vector<std::string> raw_listing;
  if (category == SelectedCategory::kNone) {
    std::tie(error, raw_listing) = download_and_split_on_newlines(server_url + "/homebrew_list");
  } else {
    std::tie(error, raw_listing) = download_and_split_on_newlines(server_url + "/" + g_category_names[category] + "/homebrew_list");
  }
  std::vector<Title> title_list;
  for (auto path : raw_listing) {
    title_list.push_back({
      path,
      path.substr(path.find("/") + 1),
      path.substr(0, path.find("/"))
    });
  }
  return std::make_tuple(error, title_list);
}

void sort_homebrew_list(BrowserState& state) {
  std::sort(begin(state.homebrew_listing), end(state.homebrew_listing));
  if (state.sort_order == ListingSortOrder::kAlphanumericDescending) {
    std::reverse(begin(state.homebrew_listing), end(state.homebrew_listing));
  }
  download_smdh_for_page(kServer, get_title_list_cursor(state.homebrew_listing,
      state.selected_index), state.app_info_for_current_page);
}

void switch_to_category(SelectedCategory category, BrowserState& state) {
  Result error{0};
  std::tie(error, state.homebrew_listing) = get_homebrew_listing(kServer, category);
  sort_homebrew_list(state);
  state.selected_index = 0;
  sort_homebrew_list(state);
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

  BrowserState state;

  //*
  switch_to_category(state.selected_category, state);

  //*
  
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
    u32 const old_selected_index = state.selected_index;
    if (kDown & KEY_UP) {
      state.selected_index = state.selected_index ? state.selected_index - 1 : 0;
    }
    if (kDown & KEY_DOWN) {
      state.selected_index = 
          state.selected_index < state.homebrew_listing.size() - 1 ?
          state.selected_index + 1 :
          state.homebrew_listing.size() - 1;
    }
    if (kDown & KEY_A) {
      download_app(kServer, state.homebrew_listing[state.selected_index].path);
    }
    if (kDown & KEY_L and state.selected_category > SelectedCategory::kNone) {
      state.selected_category = static_cast<SelectedCategory>(
          static_cast<int>(state.selected_category) - 1);
      switch_to_category(state.selected_category, state);
    }
    if (kDown & KEY_R and state.selected_category < SelectedCategory::kMisc) {
      state.selected_category = static_cast<SelectedCategory>(
          static_cast<int>(state.selected_category) + 1);
      switch_to_category(state.selected_category, state);
    }
    if (kDown & KEY_SELECT) {
      if (state.sort_order == ListingSortOrder::kAlphanumericAscending) {
        state.sort_order = ListingSortOrder::kAlphanumericDescending;
      } else {
        state.sort_order = ListingSortOrder::kAlphanumericAscending;
      }
      sort_homebrew_list(state);
    }

    if (old_selected_index / 3 != state.selected_index / 3) {
      download_smdh_for_page(kServer, 
          get_title_list_cursor(state.homebrew_listing, state.selected_index), 
          state.app_info_for_current_page);
    }

    draw_full_ui_from_state(ListingDrawState{
      state.selected_category,
      get_title_list_draw_state(
          get_title_list_cursor(state.homebrew_listing, state.selected_index), 
          state.app_info_for_current_page),
      state.selected_index % 3,
      get_scrollbar_draw_state(
          get_title_list_cursor(state.homebrew_listing, state.selected_index)),
      state.sort_order
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
