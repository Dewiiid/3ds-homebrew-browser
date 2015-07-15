#ifndef HOMEBREW_BROWSER_UI_H_
#define HOMEBREW_BROWSER_UI_H_

#include <array>
#include <vector>
#include <string>

#include <3ds.h>

namespace homebrew_browser {

enum class SelectedCategory {
  kNone = 0,
  kGames,
  kMedia,
  kEmulators,
  kTools,
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
  std::string author;
  bool owned;
};

enum class ListingScrollbarDisplay {
  kHidden = 0,
  kVisible
};

struct ListingScrollbar {
  ListingScrollbarDisplay displayed;
  s32 percentage;
  bool active;
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

struct UIElement {
  u8 const* const image;
  s32 const x;
  s32 const y;
};

#define EXPAND_UI_AS_ENUM(a, b, c, d) a,
#define EXPAND_UI_AS_STD_ARRAY(a, b, c, d) {b, c, d},
#define EXPAND_UI_AS_STRUCT(a, b, c, d) u8 a;
#define LISTING_UI_ELEMENTS(ELEMENT) \
  ELEMENT(kGamesDark, category_games_normal_bin, 4, 6) \
  ELEMENT(kGamesLight, category_games_selected_bin, 4, 6) \
  ELEMENT(kMediaDark, category_media_normal_bin, 4, 48) \
  ELEMENT(kMediaLight, category_media_selected_bin, 4, 48) \
  ELEMENT(kEmulatorsDark, category_emulators_normal_bin, 4, 90) \
  ELEMENT(kEmulatorsLight, category_emulators_selected_bin, 4, 90) \
  ELEMENT(kToolsDark, category_tools_normal_bin, 4, 132) \
  ELEMENT(kToolsLight, category_tools_selected_bin, 4, 132) \
  ELEMENT(kMiscDark, category_misc_normal_bin, 4, 174) \
  ELEMENT(kMiscLight, category_misc_selected_bin, 4, 174) \
  ELEMENT(kTopRowLight, row_base_bin, 51, 3) \
  ELEMENT(kMiddleRowLight, row_base_bin, 51, 74) \
  ELEMENT(kBottomRowLight, row_base_bin, 51, 145) \
  ELEMENT(kTopRowDark, row_selected_bin, 51, 3) \
  ELEMENT(kMiddleRowDark, row_selected_bin, 51, 74) \
  ELEMENT(kBottomRowDark, row_selected_bin, 51, 145) \
  ELEMENT(kUIBar, ui_bar_bin, 0, 216) \
  ELEMENT(kSortReversed, sort_reversed_bin, 265, 218) \
  ELEMENT(kScrollBar, scrollbar_bin, 304, 3) \
  ELEMENT(kScrollBarActive, scrollbar_active_bin, 304, 3) \
  ELEMENT(kTopOwnedIcon, owned_icon_bin, 287, 6) \
  ELEMENT(kMiddleOwnedIcon, owned_icon_bin, 287, 77) \
  ELEMENT(kBottomOwnedIcon, owned_icon_bin, 287, 148) \
  ELEMENT(kDownloadWindow, download_window_bin, 32, 32) \
  ELEMENT(kProgressBarEmpty, progress_bar_empty_bin, 85, 146) \
  ELEMENT(kProgressBarFull, progress_bar_full_bin, 85, 146)

enum class ListingUIElements {
  LISTING_UI_ELEMENTS(EXPAND_UI_AS_ENUM)
};

struct ListingUIElementSize {
  LISTING_UI_ELEMENTS(EXPAND_UI_AS_STRUCT)
};

extern std::array<UIElement, sizeof(ListingUIElementSize)> const g_listing_ui_elements;

void draw_full_ui_from_state(ListingDrawState const& state);
void redraw_full_ui();
void draw_ui_element(u8* framebuffer, ListingUIElements const element);

}  // namespace homebrew_browser

#endif  // HOMEBREW_BROWSER_UI_H_
