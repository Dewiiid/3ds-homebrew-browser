#ifndef HOMEBREW_BROWSER_STORAGE_H_
#define HOMEBREW_BROWSER_STORAGE_H_

#include <string>
#include <vector>
#include <tuple>

#include <3ds.h>

void initialize_storage();
u32 write_file(const char *path, const char *filename, void *data, u32 byte_count);
u32 write_file(std::string const& absolute_path, void* data, u32 byte_count);
std::tuple<Result, std::vector<u8>> read_entire_file(std::string const& absolute_filename);

Result mkdirp(std::string const& absolute_path);

bool file_exists(std::string const& absolute_filename);

#endif  // HOMEBREW_BROWSER_STORAGE_H_
