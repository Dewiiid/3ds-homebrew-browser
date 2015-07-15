#ifndef HOMEBREW_BROWSER_BROWSER_H_
#define HOMEBREW_BROWSER_BROWSER_H_

#include <3ds.h>

#include <string>
#include <vector>
#include <array>

#include "ui.h"

namespace homebrew_browser {
//std::string const kServer = "http://23.21.136.4:1337";
//std::string const kServer = "http://homebrewbrowser.darknovagames.com";
//std::string const kServer = "http://storage.googleapis.com/3ds-browser-apps";
std::string const kServer = "http://3ds.homebrewbrowser.com";

struct Title {
  std::string path;
  std::string title_name;
  std::string category_name;
  std::string server;

  bool operator<(const Title& other) {
    return title_name < other.title_name;
  }
};

using TitleList = std::vector<Title>;
using FilteredList = std::vector<Title*>;
struct FilteredListCursor {
  typename FilteredList::const_iterator begin;
  typename FilteredList::const_iterator end;
  typename FilteredList::const_iterator selected;
};

struct AppInfo {
  std::array<u8, 48 * 48 * 3>* image;
  std::string title;  // An empty title denotes an invalid struct.
  std::string author;
  std::string description;
  bool owned;
};

struct BrowserState {
  u32 selected_index = 0;
  SelectedCategory selected_category = SelectedCategory::kNone;
  bool scrollbar_active = false;

  TitleList full_homebrew_list;
  FilteredList filtered_homebrew_list;
  bool filtered_list_dirty = true;

  std::array<AppInfo, 3> app_info_for_current_page{{
    {new std::array<u8, 48 * 48 * 3>()},
    {new std::array<u8, 48 * 48 * 3>()},
    {new std::array<u8, 48 * 48 * 3>()}
  }};
  ListingSortOrder sort_order{ListingSortOrder::kAlphanumericAscending};
};

Result download_app(std::string const& server, std::string const& title);
void switch_to_category(SelectedCategory category, BrowserState& state);
std::tuple<Result, std::vector<Title>> get_homebrew_listing(std::string const& server_url);
void sort_homebrew_list(BrowserState& state);
void filter_homebrew_list(BrowserState& state);
void update_metadata_for_page(std::string const& server,
    FilteredListCursor const& cursor, std::array<AppInfo, 3>& smdh_cache);
FilteredListCursor get_title_list_cursor(FilteredList const& titles,
    FilteredList::size_type const& offset);
void initialize_smdh_cache();

}  // namespace homebrew_browser

#endif  // HOMEBREW_BROWSER_BROWSER_H_
