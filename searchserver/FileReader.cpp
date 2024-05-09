/*
 * Copyright ©2024 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 5950 for use solely during Spring Semester 2024 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#include <fstream>

#include "./FileReader.hpp"
#include "./HttpUtils.hpp"

using std::ifstream;
using std::string;
using std::stringstream;

namespace searchserver {

bool FileReader::read_file(string* str) {
  // Read the file into memory, and store the file contents in the
  // output parameter "str."  Be careful to handle binary data
  // correctly; i.e., you probably want to use the two-argument
  // constructor to std::string (the one that includes a length as a
  // second argument).

  // TODO: implement
  ifstream file(fname_);
  if (!file.is_open()) {
    return false;
  }

  stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  *str = string(buffer.str());

  return true;
}

}  // namespace searchserver
