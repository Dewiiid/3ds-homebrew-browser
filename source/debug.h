#ifndef HOMEBREW_BROWSER_DEBUG_H_
#define HOMEBREW_BROWSER_DEBUG_H_

#include <string>
#include <3ds.h>

namespace homebrew_browser {

void debug_message(std::string const& message, bool force_frame = false);
void debug_color(u8 r, u8 g, u8 b, int delay_frames = 30);

}  // namespace homebrew_browser

#endif  // HOMEBREW_BROWSER_DEBUG_H_