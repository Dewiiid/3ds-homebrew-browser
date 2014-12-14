#ifndef HOMEBREW_BROWSER_UTIL_H_
#define HOMEBREW_BROWSER_UTIL_H_

#include <string>
#include <sstream>
#include <vector>

#include <3ds.h>

std::string string_from_bytes(std::vector<u8> const& v);

template<typename T>
std::string string_from(T const& value) {
  std::ostringstream ss;
  ss << value;
  return ss.str();
}

#endif  // HOMEBREW_BROWSER_UTIL_H_
