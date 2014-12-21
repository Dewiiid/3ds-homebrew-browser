#ifndef HOMEBREW_BROWSER_BROWSER_H_
#define HOMEBREW_BROWSER_BROWSER_H_

#include <3ds.h>

#include <string>
#include <vector>
#include <array>

#include "ui.h"

std::string const kServer = "http://23.21.136.4:1337";

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
  ListingSortOrder sort_order{ListingSortOrder::kAlphanumericAscending};
};

Result download_app(std::string const& server, std::string const& title);
void switch_to_category(SelectedCategory category, BrowserState& state);
std::tuple<Result, std::vector<Title>> get_homebrew_listing(std::string const& server_url, SelectedCategory category);
void sort_homebrew_list(BrowserState& state);
void download_smdh_for_page(std::string const& server,
    TitleListCursor const& cursor, std::array<AppInfo, 3>& smdh_cache);
TitleListCursor get_title_list_cursor(TitleList const& titles,
    TitleList::size_type const& offset);

#endif  // HOMEBREW_BROWSER_BROWSER_H_