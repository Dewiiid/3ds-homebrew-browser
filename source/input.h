#ifndef HOMEBREW_BROWSER_INPUT_H_
#define HOMEBREW_BROWSER_INPUT_H_

#include "ui.h"
#include "browser.h"

void handle_input(u32 const keys_down, touchPosition const touch_position, BrowserState& state);

#endif  // HOMEBREW_BROWSER_INPUT_H_