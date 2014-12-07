#include "util.h"

#include <3ds.h>

std::string string_from_bytes(std::vector<u8> const& v) {
  char const* const beginning = reinterpret_cast<char const*>(&v[0]);
  return std::string{beginning, beginning + v.size()};
}
