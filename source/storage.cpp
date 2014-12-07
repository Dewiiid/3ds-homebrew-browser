#include "storage.h"

#include <cstring>

FS_archive sdmc_archive;

void initialize_storage() {
  sdmc_archive = (FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
  FSUSER_OpenArchive(NULL, &sdmc_archive);
}

u32 write_file(const char *path, const char *filename, void *data, u32 byte_count) {
  FSUSER_CreateDirectory(NULL, sdmc_archive, FS_makePath(PATH_CHAR, path));
  Handle file_handle;
  char absolute_filename[256] = {};
  strcpy(absolute_filename, path);
  strcat(absolute_filename, filename);
  FSUSER_OpenFile(NULL, &file_handle, sdmc_archive, FS_makePath(PATH_CHAR, absolute_filename), FS_OPEN_WRITE | FS_OPEN_CREATE, 0);
  u32 bytes_written = 0;
  u32 write_offset_bytes = 0;
  FSFILE_Write(file_handle, &bytes_written, write_offset_bytes, data, byte_count, FS_WRITE_FLUSH);
  FSFILE_Close(file_handle);
  return bytes_written;
}

u32 write_file(std::string const& absolute_path, void* data, u32 byte_count) {
  size_t const position_of_last_slash = absolute_path.find_last_of('/');
  std::string const directory = absolute_path.substr(0, position_of_last_slash + 1);
  std::string const filename = absolute_path.substr(position_of_last_slash + 1);
  return write_file(directory.c_str(), filename.c_str(), data, byte_count);
}
