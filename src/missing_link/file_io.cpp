/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cerrno>
#include "missing_link/file_io.hpp"

using namespace MissingLink;
using namespace MissingLink::FileIO;
using namespace std;

TextFile::TextFile(string path)
  : m_path(path)
  , m_fd(-1)
{
  //add setup here
  //check for existence of file
  //create file if it doesn't exist and write default status to file
}

TextFile::~TextFile() {
}

void TextFile::Write(string message) {
  write(message);
}

string TextFile::Read() {
  return read();
}

void TextFile::write(string message) {
  m_fd = ::open(m_path.c_str(), O_WRONLY);
  ::write(m_fd, message.c_str(), message.size());
  ::close(m_fd);
}

string TextFile::read() {
  string lastLine = "";
  string line = "";
  std::ifstream myFile;
  myFile.open(m_path, std::ios::in);
  if (myFile.is_open()) {
    while ( std::getline (myFile, line) )
      {
        lastLine = line;
      }
      myFile.close();
  }
  return lastLine;
  // m_fd = ::open(m_path.c_str(), O_RDONLY | O_NONBLOCK);
  // ::read(m_fd, &str, 80);
  // ::close(m_fd);
  // return std::string(str);
}
