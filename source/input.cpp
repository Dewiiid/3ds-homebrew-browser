#include "input.h"
#include "drawing.h"
#include "util.h"
#include "debug.h"

#include <3ds.h>

void toggle_sort_order(BrowserState& state) {
  if (state.sort_order == ListingSortOrder::kAlphanumericAscending) {
    state.sort_order = ListingSortOrder::kAlphanumericDescending;
  } else {
    state.sort_order = ListingSortOrder::kAlphanumericAscending;
  }
  sort_homebrew_list(state);
}

void handle_button_input(u32 const keys_down, BrowserState& state) {
  //Exit the app on home button
  if (keys_down & KEY_START) {
      aptSetStatus(APP_EXITING);
  }

  if (keys_down & KEY_UP || keys_down & KEY_CPAD_UP) {
      state.selected_index = state.selected_index ? state.selected_index - 1 : 0;
    }
    if (keys_down & KEY_DOWN) {
      state.selected_index = 
          state.selected_index < state.homebrew_listing.size() - 1 ?
          state.selected_index + 1 :
          state.homebrew_listing.size() - 1;
    }

    if (keys_down & KEY_A) {
      download_app(state.homebrew_listing[state.selected_index].server, 
          state.homebrew_listing[state.selected_index].path);
    }

    if (keys_down & KEY_X) {
      consoleInit(GFX_TOP, nullptr);
    }

    if (keys_down & KEY_L and state.selected_category > SelectedCategory::kNone) {
      state.selected_category = static_cast<SelectedCategory>(
          static_cast<int>(state.selected_category) - 1);
      switch_to_category(state.selected_category, state);
    }
    if (keys_down & KEY_R and state.selected_category < SelectedCategory::kMisc) {
      state.selected_category = static_cast<SelectedCategory>(
          static_cast<int>(state.selected_category) + 1);
      switch_to_category(state.selected_category, state);
    }

    if (keys_down & KEY_SELECT) {
      toggle_sort_order(state);
    }
}

bool region_touched(touchPosition const pos, int x, int y, int width, int height) {
  if (pos.px >= x and pos.py >= y and pos.px < x + width and pos.py < y + height) {
    return true;
  }
  return false;
}

bool ui_element_touched(touchPosition const pos, ListingUIElements const element) {
  UIElement const& data = g_listing_ui_elements[static_cast<size_t>(element)];
  return region_touched(pos, data.x, data.y, 
      get_image_width(data.image), get_image_height(data.image));
}

void toggle_category(SelectedCategory touched_category, BrowserState& state) {
  bool category_already_selected = touched_category == state.selected_category;
  switch_to_category(touched_category == state.selected_category ? 
      SelectedCategory::kNone : touched_category, state);
}

void handle_touch_input(touchPosition const pos, BrowserState& state) {
  if (ui_element_touched(pos, ListingUIElements::kSortReversed)) {
    toggle_sort_order(state);
  }
  if (ui_element_touched(pos, ListingUIElements::kGamesDark)) {
    toggle_category(SelectedCategory::kGames, state);
  }
  if (ui_element_touched(pos, ListingUIElements::kMediaDark)) {
    toggle_category(SelectedCategory::kMedia, state);
  }
  if (ui_element_touched(pos, ListingUIElements::kEmulatorsDark)) {
    toggle_category(SelectedCategory::kEmulators, state);
  }
  if (ui_element_touched(pos, ListingUIElements::kToolsDark)) {
    toggle_category(SelectedCategory::kTools, state);
  }
  if (ui_element_touched(pos, ListingUIElements::kMiscDark)) {
    toggle_category(SelectedCategory::kMisc, state);
  }

  debug_message("Touch: " + string_from<int>(pos.px) + ", " + string_from<int>(pos.py));
}

void handle_input(u32 const keys_down, touchPosition const touch_position, BrowserState& state) {
  handle_button_input(keys_down, state);
  if (keys_down and KEY_TOUCH) {
    handle_touch_input(touch_position, state);
  }
}