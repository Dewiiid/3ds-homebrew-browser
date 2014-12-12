#include "font.h"

#include "drawing.h"

#include "ubuntu_condensed_16px_bin.h"

Font const title_font{ubuntu_condensed_desc, ubuntu_condensed_16px_bin};

void putchar(u8* const framebuffer, s32 const x, s32 const y, Font const& font,
    char const c) {
  CharacterDescription const& character{font.offsets[c]};
  draw_sprite_from_atlas(font.atlas, framebuffer, x, y, character.x,
      character.y, character.w, character.h);
}

void putnchar(u8* const framebuffer, s32 const x, s32 const y, Font const& font,
    char const* const s, u32 const n) {
  s32 horizontal_offset = 0;
  for (u32 character_offset = 0; character_offset < n; ++character_offset) {
    char const c = s[character_offset];
    CharacterDescription const& character{font.offsets[c]};
    putchar(framebuffer, x + horizontal_offset, y, font, c);
    horizontal_offset += character.xa;
  }
}
