#ifndef HOMEBREW_BROWSER_DEBUG_H_
#define HOMEBREW_BROWSER_DEBUG_H_

#include <string>

void debug_message(std::string const& message, bool force_frame = false);
void draw_debug_area();

#endif  // HOMEBREW_BROWSER_DEBUG_H_