#ifndef HOMEBREW_BROWSER_FONT_H_
#define HOMEBREW_BROWSER_FONT_H_

#include <array>

#include <3ds.h>

struct CharacterDescription {
  char c;
  int x;
  int y;
  int w;
  int h;
  int xo;
  int yo;
  int xa;
};

struct Font {
  std::array<CharacterDescription, 128> const& offsets;
  u8 const* const atlas;
};

extern Font const title_font;
extern Font const description_font;

void putchar(u8* const framebuffer, s32 const x, s32 const y, Font const& font,
    char const c);
void putnchar(u8* const framebuffer, s32 const x, s32 const y, Font const& font,
    char const* const s, u32 const n);

u32 string_width(Font const& font, char const* const s, u32 const n);

#endif  // HOMEBREW_BROWSER_FONT_H_
