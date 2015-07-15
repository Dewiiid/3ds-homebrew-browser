#include "font.h"

#include "drawing.h"

#include "ubuntu_condensed_14pt_bin.h"
#include "ubuntu_light_10pt_bin.h"
#include "ubuntu_light_10pt_red_bin.h"

#include <sstream>
#include <deque>

using std::string;
using std::vector;

namespace hbb = homebrew_browser;

//extern std::array<CharacterDescription, 128> ubuntu_condensed_desc;
extern std::array<hbb::CharacterDescription, 128> ubuntu_light_10pt_desc;
extern std::array<hbb::CharacterDescription, 128> ubuntu_condensed_14pt_desc;

hbb::Font const hbb::title_font{ubuntu_condensed_14pt_desc, ubuntu_condensed_14pt_bin};
hbb::Font const hbb::description_font{ubuntu_light_10pt_desc, ubuntu_light_10pt_bin};
hbb::Font const hbb::author_font{ubuntu_light_10pt_desc, ubuntu_light_10pt_red_bin};

void hbb::_putchar(u8* const framebuffer, s32 const x, s32 const y, Font const& font,
    char const c) {
  CharacterDescription const& character{font.offsets[c]};
  draw_sprite_from_atlas(font.atlas, framebuffer,
      x + character.xo, y + character.yo,
      character.x, character.y,
      character.w, character.h);
}

void hbb::_putnchar(u8* const framebuffer, s32 const x, s32 const y, Font const& font,
    char const* const s, u32 const n) {
  s32 horizontal_offset = 0;
  for (u32 character_offset = 0; character_offset < n; ++character_offset) {
    char const c = s[character_offset];
    CharacterDescription const& character{font.offsets[c]};
    if (c != ' ') {
      _putchar(framebuffer, x + horizontal_offset, y, font, c);
    }
    horizontal_offset += character.xa;
  }
}

u32 hbb::string_width(Font const& font, char const* const s, u32 const n) {
  u32 width = 0;
  for (u32 character_offset = 0; character_offset < n; character_offset++) {
    char const c = s[character_offset];
    width += font.offsets[c].xa;
  }
  return width;
}

// Given an input string, returns a vector of all words in the string, broken
// up by spaces. (any other whitespace is ignored.) Intentionally separates
// multiple spaces, returning empty "words" in the list.
vector<string> split(string input) {
  std::istringstream in(input);
  vector<string> output;
  string word;
  while (in.good()) {
    getline(in, word, ' ');
    output.push_back(word);
  }
  return output;
}

// Given an input string, performs a word wrap operation, and returns a vector
// of lines which will fit inside the specified area.
vector<string> hbb::word_wrap(Font const& font, string input_string, u32 width, u32 max_lines) {
  vector<string> split_string = split(input_string);
  std::deque<string> words(begin(split_string), end(split_string));
  /*std::deque<string> words;
  words.push_back("The");
  words.push_back("quick");
  words.push_back("brown");
  words.push_back("fox");
  words.push_back("jumps");
  words.push_back("over");
  words.push_back("the");
  words.push_back("lazy");
  words.push_back("dog.");*/


  vector<string> output;
  
  u32 current_width = 0;
  string current_line = "";
  u32 space_width = string_width(font, " ", 1);
  while (words.size() > 0 and output.size() < max_lines) {
    u32 word_width = 0;
    if (current_width != 0) {
      //account for a space
      word_width += space_width;
    }
    string word = words.front();
    words.pop_front();

    word_width += string_width(font, word.c_str(), word.size());
    if (current_width + word_width <= width) {
      // Add this word to the end of the current line
      if (current_width != 0) {
        current_line += " ";
      }
      current_line += word;
      current_width += word_width;
    } else {
      // Use this word to start a new line
      output.push_back(current_line);
      current_line = word;
      current_width = word_width;
    }
  }
  output.push_back(current_line);
  return output;
}

void hbb::textbox(u8* fb, u32 x, u32 y, u32 width, u32 height, u32 spacing, Font const& font, string text) {
  u32 max_lines = height / spacing;
  vector<string> lines = word_wrap(font, text, width, max_lines);
  for (auto line : lines) {
    _putnchar(fb, x, y, font, line.c_str(), line.size());
    y += spacing;
  }
}

void hbb::_putnchar_r(u8* const framebuffer, s32 const x, s32 const y,
    Font const& font, char const* const s, u32 const n) {
  int adjusted_x = x - string_width(font, s, n);
  _putnchar(framebuffer, adjusted_x, y, font, s, n);
}