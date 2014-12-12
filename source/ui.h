#ifndef HOMEBREW_BROWSER_UI_H_
#define HOMEBREW_BROWSER_UI_H_

#include <array>
#include <string>

#include <3ds.h>

enum class SelectedCategory {
  kNone = 0,
  kGames,
  kMedia,
  kEmulators,
  kUtility,
  kMisc
};

enum class ListingTitleDisplay {
  kHidden = 0,
  kVisible
};

struct ListingMetadata {
  ListingTitleDisplay displayed;
  u8 const* icon;
  std::string title;
  std::string description;
};

enum class ListingScrollbarDisplay {
  kHidden = 0,
  kVisible
};

struct ListingScrollbar {
  ListingScrollbarDisplay displayed;
  s32 percentage;
};

enum class ListingSortOrder {
  kAlphanumericAscending = 0,
  kAlphanumericDescending
};

struct ListingDrawState {
  SelectedCategory category;
  // Icons/titles/descriptions
  std::array<ListingMetadata, 3> visible_titles;
  // Selected title
  u32 selected_title;
  // scrollbar location, whether it's displayed
  ListingScrollbar scrollbar;
  // sort state
  ListingSortOrder sort_order;
};

void draw_full_ui_from_state(ListingDrawState const& state);
void redraw_full_ui();

#endif  // HOMEBREW_BROWSER_UI_H_
