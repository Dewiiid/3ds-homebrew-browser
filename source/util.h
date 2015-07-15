#ifndef HOMEBREW_BROWSER_UTIL_H_
#define HOMEBREW_BROWSER_UTIL_H_

#include <string>
#include <sstream>
#include <vector>

#include <3ds.h>

namespace homebrew_browser {

std::string string_from_bytes(std::vector<u8> const& v);

template<typename T>
std::string string_from(T const& value, bool usehex = false) {
  std::ostringstream ss;
  if (usehex) {
    ss << "0x" << std::hex << value;
  } else {
    ss << value;
  }
  return ss.str();
}

}  // namespace homebrew_browser

#endif  // HOMEBREW_BROWSER_UTIL_H_
