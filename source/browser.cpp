#include "browser.h"

#include "http.h"
#include "storage.h"
#include "smdh.h"
#include "string.h"

#include "drawing.h"
#include "ui.h"
#include "debug.h"
#include "util.h"
#include "font.h"

#include <map>
#include <algorithm>

#include "no_icon_bin.h"

using std::string;

const string kCachePrefix = "/3ds/homebrew-browser/icon-cache/";

void initialize_smdh_cache() {
  mkdirp(kCachePrefix);
}

void prepare_download_window() {
  //Prepare the framebuffers by darkening both of them
  u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
  fx_darken_background(fb, 320*240);
  gfxFlushBuffers();
  gfxSwapBuffers();
  fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
  fx_darken_background(fb, 320*240);
}

void draw_centered_string(u8* fb, Font const& font, int y_pos, string str) {
  u32 width = string_width(font, str.c_str(), str.size());
  u32 x_pos = 160 - width / 2;
  putnchar(fb, x_pos, y_pos, font, str.c_str(), str.size());
}

void update_download_status(string current_file, int file_index, 
    int total_files, int file_progress) {
  gspWaitForVBlank();
  //first, draw a blank window
  u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
  draw_ui_element(fb, ListingUIElements::kDownloadWindow);

  draw_centered_string(fb, title_font, 70, "Currently Downloading:");
  draw_centered_string(fb, description_font, 90, current_file);
  draw_centered_string(fb, description_font, 110, "(" + 
      string_from<int>(file_index + 1) + "/" + string_from<int>(total_files) + ")");
  
  draw_ui_element(fb, ListingUIElements::kProgressBarEmpty);

  //manual draw here
  UIElement const& data = g_listing_ui_elements[static_cast<size_t>(ListingUIElements::kProgressBarFull)];
  draw_raw_sprite(data.image + 8, fb, data.x, data.y, file_progress * 150 / 100, 27);

  gfxFlushBuffers();
  gfxSwapBuffers();

}

Result download_app(std::string const& server, std::string const& title) {
  // cheat: draw an interface here, since we're going to be blocking
  // the main thread.
  // TODO: make downloads happen in the background, and instead do most of
  // this logic as part of the UI framework / application state.
  prepare_download_window();

  string appname = title.substr(title.find("/") + 1);
  Result error{0};
  std::vector<std::string> title_file_listing;
  std::tie(error, title_file_listing) =
      get_file_listing_for_title(server, title);

  //for (auto const& relative_path : title_file_listing) {
  for (unsigned int i = 0; i < title_file_listing.size(); i++) {
    auto relative_path = title_file_listing[i];
    string server_path = "/3ds/" + title + "/" + relative_path;
    update_download_status(title + "/" + relative_path, i, 
        title_file_listing.size(), 0);
    gfxFlushBuffers();
    gfxSwapBuffers();
    
    mkdirp("/3ds/" + appname);
    download_to_file(server + server_path, "/3ds/" + appname + "/" + relative_path,
      [&](int progress) {
        update_download_status(title + "/" + relative_path, i, 
          title_file_listing.size(), progress);
      });
  }
  return error;
}

Result download_smdh(std::string const& server, Title const& title, AppInfo& app_info) {
  Result error{0};
  std::vector<u8> smdh_byte_buffer;

  //check the cache first
  if (file_exists(kCachePrefix + title.title_name + ".smdh")) {
    //We already have this in the cache, so just use that.
    std::tie(error, smdh_byte_buffer) = read_entire_file(kCachePrefix 
        + title.title_name + ".smdh");
  } else {
    std::tie(error, smdh_byte_buffer) = http_get(server + "/" + title.path + "/smdh");
    if (error) {
      //provide dummy tile data, and as sane default info as we can come up with
      app_info.title = title.title_name;
      app_info.author = "Unknown Publisher";
      app_info.description = "";
      memcpy(&(*app_info.image)[0], no_icon_bin + 8, 48 * 48 * 3);
      return error;
    } else {
      //now that we have this file, go ahead and store it in the cache for later
      write_file(kCachePrefix + title.title_name + ".smdh", 
          &smdh_byte_buffer[0], smdh_byte_buffer.size());
    }
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

void update_metadata_for_page(std::string const& server,
    FilteredListCursor const& cursor, std::array<AppInfo, 3>& smdh_cache) {
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

  auto app_info = begin(smdh_cache);
  auto visible_title = visible_titles_begin;
  for (; app_info != end(smdh_cache) and visible_title != visible_titles_end;
      ++app_info, ++visible_title) {
    //update the SMDH data
    Result error = download_smdh(server, **visible_title, *app_info);
    if (error) {
      debug_message("SMDH Download ERR: " + string_from<int>(error));
    }
    if (directory_exists("/3ds/" + (*visible_title)->title_name)) {
      app_info->owned = true;
    } else {
      app_info->owned = false;
    }
  }
}

void switch_to_category(SelectedCategory category, BrowserState& state) {
  state.selected_index = 0;
  state.selected_category = category;
  state.filtered_list_dirty = true;
}

FilteredListCursor get_title_list_cursor(FilteredList const& titles,
    FilteredList::size_type const& offset) {
  return FilteredListCursor{
    begin(titles),
    end(titles),
    std::next(begin(titles), offset)
  };
}

std::map<SelectedCategory, string> g_category_names {
  {SelectedCategory::kGames, "games"},
  {SelectedCategory::kMedia, "media"},
  {SelectedCategory::kEmulators, "emulators"},
  {SelectedCategory::kTools, "tools"},
  {SelectedCategory::kMisc, "misc"}
};

std::tuple<Result, std::vector<Title>> get_homebrew_listing(std::string const& server_url) {
  Result error;
  std::vector<std::string> raw_listing;
  std::tie(error, raw_listing) = download_and_split_on_newlines(server_url + "/homebrew_list");
  std::vector<Title> title_list;
  for (auto path : raw_listing) {
    title_list.push_back({
      path,
      path.substr(path.find("/") + 1),
      path.substr(0, path.find("/")),
      server_url
    });
  }
  return std::make_tuple(error, title_list);
}

void sort_homebrew_list(BrowserState& state) {
  std::sort(begin(state.full_homebrew_list), end(state.full_homebrew_list));
  if (state.sort_order == ListingSortOrder::kAlphanumericDescending) {
    std::reverse(begin(state.full_homebrew_list), end(state.full_homebrew_list));
  }
}

void filter_homebrew_list(BrowserState& state) {
  state.filtered_homebrew_list.clear();
  for (auto& title : state.full_homebrew_list) {
    if (state.selected_category == SelectedCategory::kNone or 
        title.category_name == g_category_names[state.selected_category]) {
      state.filtered_homebrew_list.push_back(&title);
      //debug_message("Filter selected " + title.title_name);
    }
  }
}