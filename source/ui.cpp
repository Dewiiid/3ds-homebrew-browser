#include "ui.h"

#include <array>

#include <3ds.h>

#include "drawing.h"
#include "font.h"

#include "category_emulators_normal_bin.h"
#include "category_emulators_selected_bin.h"
#include "category_games_normal_bin.h"
#include "category_games_selected_bin.h"
#include "category_media_normal_bin.h"
#include "category_media_selected_bin.h"
#include "category_misc_normal_bin.h"
#include "category_misc_selected_bin.h"
#include "category_tools_normal_bin.h"
#include "category_tools_selected_bin.h"
#include "row_base_bin.h"
#include "row_selected_bin.h"
#include "scrollbar_bin.h"
#include "sort_normal_bin.h"
#include "sort_reversed_bin.h"
#include "ui_bar_bin.h"
#include "owned_icon_bin.h"
#include "download_window_bin.h"
#include "progress_bar_empty_bin.h"
#include "progress_bar_full_bin.h"

std::array<UIElement, sizeof(ListingUIElementSize)> const g_listing_ui_elements{{
  LISTING_UI_ELEMENTS(EXPAND_UI_AS_STD_ARRAY)
}};

void draw_ui_element(u8* framebuffer, ListingUIElements const element) {
  UIElement const& data = g_listing_ui_elements[static_cast<size_t>(element)];
  draw_sprite(data.image, framebuffer, data.x, data.y);
}

void draw_full_ui_from_state(ListingDrawState const& state) {
    u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
    draw_solid_background(fb, 240 * 320, 216, 201, 201);

    draw_ui_element(fb, state.category == SelectedCategory::kGames ? ListingUIElements::kGamesLight : ListingUIElements::kGamesDark);
    draw_ui_element(fb, state.category == SelectedCategory::kMedia ? ListingUIElements::kMediaLight : ListingUIElements::kMediaDark);
    draw_ui_element(fb, state.category == SelectedCategory::kEmulators ? ListingUIElements::kEmulatorsLight : ListingUIElements::kEmulatorsDark);
    draw_ui_element(fb, state.category == SelectedCategory::kTools ? ListingUIElements::kToolsLight : ListingUIElements::kToolsDark);
    draw_ui_element(fb, state.category == SelectedCategory::kMisc ? ListingUIElements::kMiscLight : ListingUIElements::kMiscDark);

    if (state.visible_titles[0].displayed == ListingTitleDisplay::kVisible) {
      draw_ui_element(fb, state.selected_title == 0 ? ListingUIElements::kTopRowDark : ListingUIElements::kTopRowLight);
      draw_raw_sprite(state.visible_titles[0].icon, fb, 61, 13, 48, 48);
      putnchar(fb, 123, 9, title_font, state.visible_titles[0].title.c_str(), state.visible_titles[0].title.size());
      if (state.visible_titles[0].owned) {
        draw_ui_element(fb, ListingUIElements::kTopOwnedIcon);
      }
    }
    if (state.visible_titles[1].displayed == ListingTitleDisplay::kVisible) {
      draw_ui_element(fb, state.selected_title == 1 ? ListingUIElements::kMiddleRowDark : ListingUIElements::kMiddleRowLight);
      draw_raw_sprite(state.visible_titles[1].icon, fb, 61, 84, 48, 48);
      putnchar(fb, 123, 80, title_font, state.visible_titles[1].title.c_str(), state.visible_titles[1].title.size());
      if (state.visible_titles[1].owned) {
        draw_ui_element(fb, ListingUIElements::kMiddleOwnedIcon);
      }
    }
    if (state.visible_titles[2].displayed == ListingTitleDisplay::kVisible) {
      draw_ui_element(fb, state.selected_title == 2 ? ListingUIElements::kBottomRowDark : ListingUIElements::kBottomRowLight);
      draw_raw_sprite(state.visible_titles[2].icon, fb, 61, 155, 48, 48);
      putnchar(fb, 123, 151, title_font, state.visible_titles[2].title.c_str(), state.visible_titles[2].title.size());
      if (state.visible_titles[2].owned) {
        draw_ui_element(fb, ListingUIElements::kBottomOwnedIcon);
      }
    }

    draw_ui_element(fb, ListingUIElements::kUIBar);
    if (state.sort_order == ListingSortOrder::kAlphanumericDescending) {
      draw_ui_element(fb, ListingUIElements::kSortReversed);
    }

    if (state.scrollbar.displayed == ListingScrollbarDisplay::kVisible) {
      UIElement const& data = g_listing_ui_elements[static_cast<size_t>(ListingUIElements::kScrollBar)];
      // Range is [3,153], total of 150 potential values
      // percentage / 100 = position / potential
      // percentage / 100 * potential = position
      // Reorder the multiply and divide so that integer truncation is not an issue.
      s32 const scroll_offset = state.scrollbar.percentage * 150 / 100;
      draw_sprite(data.image, fb, data.x, data.y + scroll_offset);
    }
}

void redraw_full_ui() {
    u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
    draw_solid_background(fb, 240 * 320, 216, 201, 201);

    draw_ui_element(fb, ListingUIElements::kGamesDark);
    draw_ui_element(fb, ListingUIElements::kMediaDark);
    draw_ui_element(fb, ListingUIElements::kEmulatorsDark);
    draw_ui_element(fb, ListingUIElements::kToolsDark);
    draw_ui_element(fb, ListingUIElements::kMiscDark);

    draw_ui_element(fb, ListingUIElements::kTopRowLight);
    draw_ui_element(fb, ListingUIElements::kMiddleRowLight);
    draw_ui_element(fb, ListingUIElements::kBottomRowLight);

    draw_ui_element(fb, ListingUIElements::kUIBar);
    draw_ui_element(fb, ListingUIElements::kSortReversed);

    draw_ui_element(fb, ListingUIElements::kScrollBar);
}
