#include "debug.h"
#include "drawing.h"

#include <cstring>
#include <deque>
#include <iostream>

#include <3ds.h>
#include "font.h"

using namespace std;

namespace hbb = homebrew_browser;

const int kLineHeight = 16; //pixels to scroll
const int kTopScreenHeight = 240;
const int kTopScreenWidth = 400;

void hbb::debug_message(string const& message, bool force_frame) {
  cerr << message << endl;

  if (force_frame) {
    gspWaitForVBlank();

    // Flush and swap framebuffers
    gfxFlushBuffers();
    gfxSwapBuffers();
  }
}

void hbb::debug_color(u8 r, u8 g, u8 b, int delay_frames) {
  for (int i = 0; i < delay_frames; ++i)
  {
    u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
    draw_solid_background(fb, 240 * 320, r, g, b);
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
  }
}