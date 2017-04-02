#pragma once

#include <string>

namespace MissingLink {

class File {
public:

  enum Access {
    WRITE = 0x01,
    READ = 0x02
  };

  File(const std::string strPath, const Access access);
  virtual ~File();

  int Open();
  int Close();

  int Seek(const size_t nBytes);

  // On success returns bytes read.
  // On error returns negative value.
  int Read(char *buf, const size_t nBytes);

  int Write(const char *buf, const size_t nBytes);
  int Write(const std::string strValue);

protected:

  const std::string m_strPath;
  const Access m_access;

  virtual bool isOpen() const = 0;

  bool canRead() const;
  bool canWrite() const;

  virtual int open() = 0;
  virtual int close() = 0;
  virtual int seek(const size_t nBytes) = 0;
  virtual int write(const char *buf, const size_t nBytes) = 0;
  virtual int read(char *buf, const size_t nBytes) = 0;
};

class UnbufferedFile : public File {
public:

  UnbufferedFile(const std::string strPath, const Access access);
  ~UnbufferedFile();

private:

  int m_fd;

  bool isOpen() const override;
  int open() override;
  int close() override;
  int seek(const size_t nBytes) override;
  int write(const char *buf, const size_t nBytes) override;
  int read(char *buf, const size_t nBytes) override;
};

}
