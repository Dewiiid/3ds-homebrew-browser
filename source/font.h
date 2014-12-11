#ifndef HOMEBREW_BROWSER_FONT_H_
#define HOMEBREW_BROWSER_FONT_H_

#include <array>

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

extern std::array<CharacterDescription, 128> ubuntu_condensed_desc;

#endif  // HOMEBREW_BROWSER_FONT_H_
