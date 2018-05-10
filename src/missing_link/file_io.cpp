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
  if (!exists()) {
    write("TRYING_TO_CONNECT");
  }
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
  std::ofstream myFile;
  myFile.open(m_path, std::ofstream::out | std::ofstream::trunc);
  myFile << message;
  myFile.close();
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
}

bool TextFile::exists() {
  ifstream f(m_path.c_str());
  return f.good();
}
