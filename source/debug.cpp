#include "debug.h"
#include "drawing.h"

#include <cstring>
#include <deque>

#include <3ds.h>
#include "font.h"

using namespace std;

const int kLineHeight = 16; //pixels to scroll
const int kTopScreenHeight = 240;
const int kTopScreenWidth = 400;

int g_debug_dirty = 0;

void init_debug_area() {
  //Draw the top screens WHITE
  u8* top_lefteye = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
  memset(top_lefteye, 0xFF, kTopScreenHeight*kTopScreenWidth*3);
}

deque<string> debug_lines;

void draw_debug_area() {
  //if (g_debug_dirty > 0) {
  //  return;
  //}
  u8* top_lefteye = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
  memset(top_lefteye, 0xFF, kTopScreenHeight * kTopScreenWidth * 3);

  init_debug_area();
  int y = 0;
  for (auto line = begin(debug_lines); line != end(debug_lines); line++) {
    putnchar(top_lefteye, 0, y, console_font, line->c_str(), line->size());
    y += kLineHeight;
  }

  g_debug_dirty -= 1;
}

void debug_message(string const& message, bool force_frame) {
  debug_lines.push_back(message);
  if (debug_lines.size() > kTopScreenHeight / kLineHeight) {
    debug_lines.pop_front();
  }

  g_debug_dirty = 2;

  if (force_frame) {
    gspWaitForVBlank();

    // Draw the debug output on the top screen
    draw_debug_area();

    // Flush and swap framebuffers
    gfxFlushBuffers();
    gfxSwapBuffers();
  }
}

void debug_color(u8 r, u8 g, u8 b, int delay_frames) {
  for (int i = 0; i < delay_frames; ++i)
  {
    u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
    draw_solid_background(fb, 240 * 320, r, g, b);
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
  }
}