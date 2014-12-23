#include "font.h"

#include "drawing.h"

#include "ubuntu_condensed_14pt_bin.h"
#include "ubuntu_light_10pt_bin.h"

//extern std::array<CharacterDescription, 128> ubuntu_condensed_desc;
extern std::array<CharacterDescription, 128> ubuntu_light_10pt_desc;
extern std::array<CharacterDescription, 128> ubuntu_condensed_14pt_desc;

Font const title_font{ubuntu_condensed_14pt_desc, ubuntu_condensed_14pt_bin};
Font const console_font{ubuntu_light_10pt_desc, ubuntu_light_10pt_bin};

void putchar(u8* const framebuffer, s32 const x, s32 const y, Font const& font,
    char const c) {
  CharacterDescription const& character{font.offsets[c]};
  draw_sprite_from_atlas(font.atlas, framebuffer,
      x + character.xo, y + character.yo,
      character.x, character.y,
      character.w, character.h);
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

u32 string_width(Font const& font, char const* const s, u32 const n) {
  u32 width = 0;
  for (u32 character_offset = 0; character_offset < n; character_offset++) {
    char const c = s[character_offset];
    width += font.offsets[c].xa;
  }
  return width;
}