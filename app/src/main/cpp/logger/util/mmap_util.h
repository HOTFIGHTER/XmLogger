#ifndef MMAP_UTIL_H_
#define MMAP_UTIL_H_

#include <boost/iostreams/device/mapped_file.hpp>
#include <logger/util/file_operator.h>
bool IsMmapFileOpenSucc(const boost::iostreams::mapped_file &_mmmap_file);

bool
OpenMmapFile(const char *_filepath, unsigned int _size, boost::iostreams::mapped_file &_mmmap_file);

void CloseMmapFile(boost::iostreams::mapped_file &_mmmap_file);


#endif /* MMAP_UTIL_H_ */
