#include "missing_link/file.hpp"
#include <iostream>

#include <fcntl.h>
#include <unistd.h>

using namespace MissingLink;
using std::string;

File::File(const string strPath, const Access access)
  : m_strPath(strPath)
  , m_access(access)
{}

File::~File() {}

int File::Open() {
  if (isOpen()) {
    std::cerr << "Cannot open file " << m_strPath << ": Already open" << std::endl;
    return -1;
  };
  int result = open();
  if (result < 0) {
    std::cerr << "Failed to open file " << m_strPath << std::endl;
  }
  return result;
}

int File::Close() {
  if (!isOpen()) {
    std::cerr << "Cannot close file " << m_strPath << ": Already closed" << std::endl;
    return -1;
  }
  int result = close();
  if (result < 0) {
    std::cerr << "Failed to close file " << m_strPath << std::endl;
  }
  return result;
}

int File::Seek(const size_t nBytes) {
  if (!isOpen()) {
    std::cerr << "Cannot seek file " << m_strPath << ": File not open" << std::endl;
    return -1;
  }
  int result = seek(nBytes);
  if (result < 0) {
    std::cerr << "Failed to seek file " << m_strPath << std::endl;
  }
  return result;
}

int File::Read(char *buf, const size_t nBytes) {
  if (!canRead()) {
    std::cerr << "Cannot read from file: opened without read access" << std::endl;
    return -1;
  }
  return read(buf, nBytes);
}

int File::Write(const char *buf, const size_t nBytes) {
  if (!canWrite()) {
    std::cerr << "Cannot write to file: opened without write access" << std::endl;
    return -1;
  }
  int result = write(buf, nBytes);
  if (result < 0) {
    std::cerr << "Failed to write to file " << m_strPath << std::endl;
  }
  return result;
}

int File::Write(const string strValue) {
  return Write(strValue.c_str(), strValue.size());
}

bool File::canRead() const {
  return m_access & READ;
}

bool File::canWrite() const {
  return m_access & WRITE;
}

// Unbuffered File based on C system calls

UnbufferedFile::UnbufferedFile(const string strPath, const Access access)
  : File(strPath, access)
  , m_fd(-1)
{}

UnbufferedFile::~UnbufferedFile() {
  if (isOpen()) {
    std::cout << "WARNING: UnbufferedFile instance destroyed before being closed: " << m_strPath << std::endl;
  }
}

bool UnbufferedFile::isOpen() const {
  return m_fd > 0;
}

int UnbufferedFile::open() {
  int accessFlag = 0;
  if (canRead() && canWrite()) {
    accessFlag = O_RDWR;
  } else if (canRead()) {
    accessFlag = O_RDONLY;
  } else if (canWrite()) {
    accessFlag = O_WRONLY;
  } else {
    std::cerr << "Invalid access options for File " <<  m_strPath << std::endl;
    return -1;
  }
  m_fd = ::open(m_strPath.c_str(), accessFlag);
  return m_fd > 0 ? 0 : -1;
}

int UnbufferedFile::close() {
  int result = ::close(m_fd);
  m_fd = -1;
  return result;
}

int UnbufferedFile::seek(const size_t nBytes) {
  bool success = ::lseek(m_fd, (off_t)nBytes, SEEK_SET);
  return success ? 0 : -1;
}

int UnbufferedFile::write(const char *buf, const size_t nBytes) {
  bool success = ::write(m_fd, buf, nBytes) == (int)nBytes;
  return success ? 0 : -1;
}

int UnbufferedFile::read(char *buf, const size_t nBytes) {
  bool success = ::read(m_fd, buf, nBytes) == (int)nBytes;
  return success ? 0 : -1;
}
