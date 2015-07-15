#include "storage.h"

#include "debug.h"
#include "util.h"

#include <cstring>

namespace hbb = homebrew_browser;

FS_archive sdmc_archive;

void hbb::initialize_storage() {
  sdmc_archive = (FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
  FSUSER_OpenArchive(NULL, &sdmc_archive);
}

u32 hbb::write_file(const char *path, const char *filename, void *data, u32 byte_count) {
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

u32 hbb::write_file(std::string const& absolute_path, void* data, u32 byte_count) {
  size_t const position_of_last_slash = absolute_path.find_last_of('/');
  std::string const directory = absolute_path.substr(0, position_of_last_slash + 1);
  std::string const filename = absolute_path.substr(position_of_last_slash + 1);
  return write_file(directory.c_str(), filename.c_str(), data, byte_count);
}

std::tuple<Result, Handle> hbb::open_file_for_writing(std::string const& absolute_filename) {
  Result error{0};
  Handle file_handle;
  error = FSUSER_OpenFile(NULL, &file_handle, sdmc_archive, FS_makePath(PATH_CHAR, absolute_filename.c_str()), FS_OPEN_WRITE | FS_OPEN_CREATE, 0);
  return std::make_tuple(error, file_handle);
}

// Attempt to make this directory, including all of its parent directories
Result hbb::mkdirp(std::string const& absolute_path) {
  //split this path into its filename and its "parent" path
  size_t const position_of_last_slash = absolute_path.find_last_of('/');
  std::string const directory = absolute_path.substr(0, position_of_last_slash);
  std::string const filename = absolute_path.substr(position_of_last_slash + 1);

  if (directory.size() > 0) {
    //make sure the parent path exists first
    mkdirp(directory);
  }

  Result error;
  error = FSUSER_CreateDirectory(NULL, sdmc_archive, 
      FS_makePath(PATH_CHAR, absolute_path.c_str()));
  return error;
}

bool hbb::file_exists(std::string const& absolute_filename) {
  Result error{0};
  Handle file_handle;
  error = FSUSER_OpenFile(NULL, &file_handle, sdmc_archive, FS_makePath(PATH_CHAR, absolute_filename.c_str()), FS_OPEN_READ, 0);
  if (!error) {
    FSFILE_Close(file_handle);
  }
  return !error;
}

bool hbb::directory_exists(std::string const& absolute_path) {
  Result error{0};
  Handle directory_handle;
  error = FSUSER_OpenDirectory(NULL, &directory_handle, sdmc_archive, FS_makePath(PATH_CHAR, absolute_path.c_str()));
  if (!error) {
    FSDIR_Close(directory_handle);
  }
  return !error;
}

// Given a filename, attempts to read the contents into a vector. Result is 0
// on success.
std::tuple<Result, std::vector<u8>> hbb::read_entire_file(std::string const& absolute_filename) {
  Result error{0};
  Handle file_handle;
  error = FSUSER_OpenFile(NULL, &file_handle, sdmc_archive, FS_makePath(PATH_CHAR, absolute_filename.c_str()), FS_OPEN_READ, 0);
  if (error) {
    //FSFILE_Close(file_handle); // maybe not needed?
    return std::make_tuple(error, std::vector<u8>{});
  }
  std::vector<u8> file_contents;
  u64 file_size;
  FSFILE_GetSize(file_handle, &file_size);
  file_contents.resize(file_size);
  u32 bytes_read;
  const u32 kOffset{0};
  FSFILE_Read(file_handle, &bytes_read, kOffset, &file_contents[0], file_contents.size());
  FSFILE_Close(file_handle);
  return std::make_tuple(error, file_contents);
}