#ifndef HOMEBREW_BROWSER_FONT_H_
#define HOMEBREW_BROWSER_FONT_H_

#include <array>
#include <vector>
#include <string>

#include <3ds.h>

namespace homebrew_browser {

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
extern Font const author_font;

void _putchar(u8* const framebuffer, s32 const x, s32 const y, Font const& font,
    char const c);
void _putnchar(u8* const framebuffer, s32 const x, s32 const y, Font const& font,
    char const* const s, u32 const n);
void _putnchar_r(u8* const framebuffer, s32 const x, s32 const y,
    Font const& font, char const* const s, u32 const n);

u32 string_width(Font const& font, char const* const s, u32 const n);
std::vector<std::string> word_wrap(Font const& font, std::string str, u32 width, u32 max_lines);
void textbox(u8* fb, u32 x, u32 y, u32 width, u32 height, u32 spacing, Font const& font, std::string text);

}  // namespace homebrew_browser

#endif  // HOMEBREW_BROWSER_FONT_H_
