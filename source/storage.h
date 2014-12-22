#ifndef HOMEBREW_BROWSER_STORAGE_H_
#define HOMEBREW_BROWSER_STORAGE_H_

#include <string>

#include <3ds.h>

void initialize_storage();
u32 write_file(const char *path, const char *filename, void *data, u32 byte_count);
u32 write_file(std::string const& absolute_path, void* data, u32 byte_count);

Result mkdirp(std::string const& absolute_path);

bool file_exists(std::string const& absolute_filename);

#endif  // HOMEBREW_BROWSER_STORAGE_H_
