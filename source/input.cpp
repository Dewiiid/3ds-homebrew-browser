#include "input.h"

#include <3ds.h>

void handle_input(u32 kDown, BrowserState& state) {
  //Exit the app on home button
  if (kDown & KEY_START) {
      aptSetStatus(APP_EXITING);
  }
  
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
      download_app(state.homebrew_listing[state.selected_index].server, 
          state.homebrew_listing[state.selected_index].path);
    }

    if (kDown & KEY_X) {
      consoleInit(GFX_TOP, nullptr);
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
}