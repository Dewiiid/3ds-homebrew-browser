#include "input.h"
#include "drawing.h"
#include "util.h"
#include "debug.h"

#include <3ds.h>

namespace hbb = homebrew_browser;

void toggle_sort_order(hbb::BrowserState& state) {
  if (state.sort_order == hbb::ListingSortOrder::kAlphanumericAscending) {
    state.sort_order = hbb::ListingSortOrder::kAlphanumericDescending;
  } else {
    state.sort_order = hbb::ListingSortOrder::kAlphanumericAscending;
  }
  state.filtered_list_dirty = true;
}

void handle_button_input(u32 const keys_down, hbb::BrowserState& state) {
  //Exit the app on home button
  if (keys_down & KEY_START) {
      aptSetStatus(APP_EXITING);
  }

  if (keys_down & KEY_UP || keys_down & KEY_CPAD_UP) {
      state.selected_index = state.selected_index ? state.selected_index - 1 : 0;
    }
    if (keys_down & KEY_DOWN) {
      state.selected_index = 
          state.selected_index < state.filtered_homebrew_list.size() - 1 ?
          state.selected_index + 1 :
          state.filtered_homebrew_list.size() - 1;
    }

    if (keys_down & KEY_A) {
      hbb::download_app(state.filtered_homebrew_list[state.selected_index]->server, 
          state.filtered_homebrew_list[state.selected_index]->path);
    }

    if (keys_down & KEY_X) {
      consoleInit(GFX_TOP, nullptr);
    }

    if (keys_down & KEY_L and state.selected_category > hbb::SelectedCategory::kNone) {
      state.selected_category = static_cast<hbb::SelectedCategory>(
          static_cast<int>(state.selected_category) - 1);
      switch_to_category(state.selected_category, state);
    }
    if (keys_down & KEY_R and state.selected_category < hbb::SelectedCategory::kMisc) {
      state.selected_category = static_cast<hbb::SelectedCategory>(
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

bool ui_element_touched(touchPosition const pos, hbb::ListingUIElements const element) {
  hbb::UIElement const& data = hbb::g_listing_ui_elements[static_cast<size_t>(element)];
  return region_touched(pos, data.x, data.y, 
      hbb::get_image_width(data.image), hbb::get_image_height(data.image));
}

void toggle_category(hbb::SelectedCategory touched_category, hbb::BrowserState& state) {
  bool category_already_selected = touched_category == state.selected_category;
  switch_to_category(category_already_selected ? 
      hbb::SelectedCategory::kNone : touched_category, state);
}

void handle_touch_regions(touchPosition const pos, hbb::BrowserState& state) {
  //UI Bar
  if (ui_element_touched(pos, hbb::ListingUIElements::kSortReversed)) {
    toggle_sort_order(state);
  }
  //Home Button
  if (region_touched(pos, 292, 219, 28, 21)) {
    aptSetStatus(APP_EXITING);
  }

  //Categories
  if (ui_element_touched(pos, hbb::ListingUIElements::kGamesDark)) {
    toggle_category(hbb::SelectedCategory::kGames, state);
  }
  if (ui_element_touched(pos, hbb::ListingUIElements::kMediaDark)) {
    toggle_category(hbb::SelectedCategory::kMedia, state);
  }
  if (ui_element_touched(pos, hbb::ListingUIElements::kEmulatorsDark)) {
    toggle_category(hbb::SelectedCategory::kEmulators, state);
  }
  if (ui_element_touched(pos, hbb::ListingUIElements::kToolsDark)) {
    toggle_category(hbb::SelectedCategory::kTools, state);
  }
  if (ui_element_touched(pos, hbb::ListingUIElements::kMiscDark)) {
    toggle_category(hbb::SelectedCategory::kMisc, state);
  }

  //App Rows - touching one initiates a download
  u32 base_index = state.selected_index - (state.selected_index % 3);
  if (ui_element_touched(pos, hbb::ListingUIElements::kTopRowLight) and
      base_index < state.filtered_homebrew_list.size()) {
    hbb::download_app(state.filtered_homebrew_list[base_index]->server, 
          state.filtered_homebrew_list[base_index]->path);
  }
  if (ui_element_touched(pos, hbb::ListingUIElements::kMiddleRowLight) and
      base_index + 1 < state.filtered_homebrew_list.size()) {
    hbb::download_app(state.filtered_homebrew_list[base_index + 1]->server, 
          state.filtered_homebrew_list[base_index + 1]->path);
  }
  if (ui_element_touched(pos, hbb::ListingUIElements::kBottomRowLight) and
      base_index + 2 < state.filtered_homebrew_list.size()) {
    hbb::download_app(state.filtered_homebrew_list[base_index + 2]->server, 
          state.filtered_homebrew_list[base_index + 2]->path);
  }  
}

int scrollbar_height() {
  return hbb::get_image_height(hbb::g_listing_ui_elements[static_cast<size_t>(
      hbb::ListingUIElements::kScrollBar)].image) - 1; // account for shadow pixel
}

int scrollbar_width() {
  return hbb::get_image_width(hbb::g_listing_ui_elements[static_cast<size_t>(
      hbb::ListingUIElements::kScrollBar)].image) - 1; // account for shadow pixel
}

s32 const kScrollAreaTop = 3;
s32 const kScrollAreaHeight = 210;
int scrollbar_current_y(hbb::BrowserState& state) {
  return kScrollAreaTop + (state.selected_index * 
      (kScrollAreaHeight - scrollbar_height()) 
      / (state.filtered_homebrew_list.size() - 1));
}

bool inside_scrollbar(hbb::BrowserState& state, touchPosition pos) {
  hbb::UIElement const& data = hbb::g_listing_ui_elements[static_cast<size_t>(
      hbb::ListingUIElements::kScrollBar)];
  return region_touched(pos, data.x, scrollbar_current_y(state), 
      scrollbar_width(), scrollbar_height());
}

void handle_touch_scrollbar(hbb::BrowserState& state, u32 keys_down, u32 keys_held, u32 keys_up, touchPosition const pos) {  
  // this stores the stylus's initial touch position, so we can offset the
  // behavior later; this makes grabbing the "top" or the "bottom" of the
  // scrollbar work, and not feel janky.
  static s32 touch_offset;
  if (keys_down & KEY_TOUCH and inside_scrollbar(state, pos)) {
    state.scrollbar_active = true;
    touch_offset = pos.py - scrollbar_current_y(state);
  }

  // drag action; while the scrollbar is active, change the selected index
  // based on the Y coordinate of the stylus
  if (keys_held & KEY_TOUCH and state.scrollbar_active) {
    s32 scrolled_index = (pos.py - kScrollAreaTop - touch_offset) 
        * (signed)state.filtered_homebrew_list.size() / 150;

    hbb::debug_message("before " + hbb::string_from<s32>(scrolled_index));
    hbb::debug_message("offset " + hbb::string_from<s32>(touch_offset));

    if (scrolled_index >= (signed)state.filtered_homebrew_list.size()) {
      scrolled_index = state.filtered_homebrew_list.size() - 1;
    }
    if (scrolled_index < 0) {
      scrolled_index = 0;
    }

    hbb::debug_message("after " + hbb::string_from<s32>(scrolled_index));

    state.selected_index = scrolled_index;
  }

  if (keys_up & KEY_TOUCH) {
    state.scrollbar_active = false;
  }
}

void hbb::handle_input(BrowserState& state) {
  hidScanInput();
  u32 keys_down = hidKeysDown();
  u32 keys_held = hidKeysHeld();
  u32 keys_up = hidKeysUp();
  touchPosition touch_position;
  hidTouchRead(&touch_position);

  // Scrolling while doing other things is fiddly and not working very
  // well, so make sure other inputs are ignored until the user lets
  // go of the scrollbar. (TODO later: investigate why this was crashing?)
  if (!state.scrollbar_active) {
    handle_button_input(keys_down, state);
    if (keys_down and KEY_TOUCH) {
      handle_touch_regions(touch_position, state);
    }
  }

  handle_touch_scrollbar(state, keys_down, keys_held, keys_up, touch_position);
}