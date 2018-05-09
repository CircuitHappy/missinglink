/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

 #pragma once

 #include <string>


 namespace MissingLink {
 namespace FileIO {

 class TextFile {

   public:

     TextFile(std::string path);
     virtual ~TextFile();

     std::string Read();
     void Write(std::string message);

   protected:

     const std::string m_path;

     int m_fd;

     void open();

     std::string read();
     void write(std::string message);

 };
}} // namespaces
