#include "mmap_util.h"
#include <unistd.h>
#include <stdio.h>
#include "boost/filesystem.hpp"

bool IsMmapFileOpenSucc(const boost::iostreams::mapped_file &_mmmap_file) {
  return !_mmmap_file.operator!() && _mmmap_file.is_open();
}

bool OpenMmapFile(const char *_filepath,
                  unsigned int _size,
                  boost::iostreams::mapped_file &_mmmap_file) {
  if (NULL == _filepath || 0 == strnlen(_filepath, 128) || 0 == _size) {
    return false;
  }

  if (IsMmapFileOpenSucc(_mmmap_file)) {
    CloseMmapFile(_mmmap_file);
  }

  if (_mmmap_file.is_open() && _mmmap_file.operator!()) {
    return false;
  }

  boost::iostreams::basic_mapped_file_params<boost::filesystem::path> param;
  param.path = boost::filesystem::path(_filepath);
  param.flags = boost::iostreams::mapped_file_base::readwrite;

  bool file_exist = isFileExists(_filepath);
  if (!file_exist) {
    param.new_file_size = _size;
  }

  _mmmap_file.open(param);

  bool is_open = IsMmapFileOpenSucc(_mmmap_file);
#ifndef _WIN32
  if (!file_exist && is_open) {
    FILE *file = fopen(_filepath, "rb+");
    if (NULL == file) {
      _mmmap_file.close();
      reMove(_filepath);
      return false;
    }

    char *zero_data = new char[_size];
    memset(zero_data, 0, _size);

    if (_size != fwrite(zero_data, sizeof(char), _size, file)) {
      _mmmap_file.close();
      fclose(file);
      reMove(_filepath);
      delete[] zero_data;
      return false;
    }
    fclose(file);
    delete[] zero_data;
  }
#endif
  return is_open;
}

void CloseMmapFile(boost::iostreams::mapped_file &_mmmap_file) {
  if (_mmmap_file.is_open()) {
    _mmmap_file.close();
  }
}
