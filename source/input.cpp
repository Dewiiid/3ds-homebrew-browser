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
  state.filtered_list_dirty = true;
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
          state.selected_index < state.filtered_homebrew_list.size() - 1 ?
          state.selected_index + 1 :
          state.filtered_homebrew_list.size() - 1;
    }

    if (keys_down & KEY_A) {
      download_app(state.filtered_homebrew_list[state.selected_index]->server, 
          state.filtered_homebrew_list[state.selected_index]->path);
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
  switch_to_category(category_already_selected ? 
      SelectedCategory::kNone : touched_category, state);
}

void handle_touch_regions(touchPosition const pos, BrowserState& state) {
  //UI Bar
  if (ui_element_touched(pos, ListingUIElements::kSortReversed)) {
    toggle_sort_order(state);
  }
  //Home Button
  if (region_touched(pos, 292, 219, 28, 21)) {
    aptSetStatus(APP_EXITING);
  }

  //Categories
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

  //App Rows - touching one initiates a download
  u32 base_index = state.selected_index - (state.selected_index % 3);
  if (ui_element_touched(pos, ListingUIElements::kTopRowLight) and
      base_index < state.filtered_homebrew_list.size()) {
    download_app(state.filtered_homebrew_list[base_index]->server, 
          state.filtered_homebrew_list[base_index]->path);
  }
  if (ui_element_touched(pos, ListingUIElements::kMiddleRowLight) and
      base_index + 1 < state.filtered_homebrew_list.size()) {
    download_app(state.filtered_homebrew_list[base_index + 1]->server, 
          state.filtered_homebrew_list[base_index + 1]->path);
  }
  if (ui_element_touched(pos, ListingUIElements::kBottomRowLight) and
      base_index + 2 < state.filtered_homebrew_list.size()) {
    download_app(state.filtered_homebrew_list[base_index + 2]->server, 
          state.filtered_homebrew_list[base_index + 2]->path);
  }  
}

int scrollbar_height() {
  return get_image_height(g_listing_ui_elements[static_cast<size_t>(
      ListingUIElements::kScrollBar)].image) - 1; // account for shadow pixel
}

int scrollbar_width() {
  return get_image_width(g_listing_ui_elements[static_cast<size_t>(
      ListingUIElements::kScrollBar)].image) - 1; // account for shadow pixel
}

s32 const kScrollAreaTop = 3;
s32 const kScrollAreaHeight = 210;
int scrollbar_current_y(BrowserState& state) {
  return kScrollAreaTop + (state.selected_index * 
      (kScrollAreaHeight - scrollbar_height()) 
      / (state.filtered_homebrew_list.size() - 1));
}

bool inside_scrollbar(BrowserState& state, touchPosition pos) {
  UIElement const& data = g_listing_ui_elements[static_cast<size_t>(
      ListingUIElements::kScrollBar)];
  return region_touched(pos, data.x, scrollbar_current_y(state), 
      scrollbar_width(), scrollbar_height());
}

void handle_touch_scrollbar(BrowserState& state, u32 keys_down, u32 keys_held, u32 keys_up, touchPosition const pos) {  
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

    debug_message("before " + string_from<s32>(scrolled_index));
    debug_message("offset " + string_from<s32>(touch_offset));

    if (scrolled_index >= (signed)state.filtered_homebrew_list.size()) {
      scrolled_index = state.filtered_homebrew_list.size() - 1;
    }
    if (scrolled_index < 0) {
      scrolled_index = 0;
    }

    debug_message("after " + string_from<s32>(scrolled_index));

    state.selected_index = scrolled_index;
  }

  if (keys_up & KEY_TOUCH) {
    state.scrollbar_active = false;
  }
}

void handle_input(BrowserState& state) {
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