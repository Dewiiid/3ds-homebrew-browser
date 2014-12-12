#include "font.h"

#include "drawing.h"

#include "ubuntu_condensed_16px_0_bin.h"

Font const title_font{ubuntu_condensed_desc, ubuntu_condensed_16px_0_bin};

void putchar(u8* const framebuffer, s32 x, s32 y, Font const& font,
    char const c) {
  draw_sprite_from_atlas(font.atlas, framebuffer, x, y, font.offsets[c].x,
      font.offsets[c].y, font.offsets[c].w, font.offsets[c].h);
}
