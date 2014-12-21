#ifndef HOMEBREW_BROWSER_BROWSER_H_
#define HOMEBREW_BROWSER_BROWSER_H_

struct Title {
  std::string path;
  std::string title_name;
  std::string category_name;

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



#endif  // HOMEBREW_BROWSER_BROWSER_H_