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

#include "./FileReader.hpp"

#include "./catch.hpp"

using std::string;

using searchserver::FileReader;


TEST_CASE("Basic", "[Test_FileReader]") {
  // See if we can read a file successfully.
  FileReader f("./test_files/hextext.txt");
  string contents;
  REQUIRE(f.read_file(&contents));
  REQUIRE(contents.size() == 4800U);

  // See if we can read a non text file
  // that contains the '\0' byte.
  f = FileReader("./test_files/transparent.gif");
  REQUIRE(f.read_file(&contents));
  REQUIRE(contents.size() == 43U);

  // Make sure reading a non-existent file fails.
  f = FileReader("./non-existent");
  REQUIRE_FALSE(f.read_file(&contents));

  // Another non-existant file, but
  // with a more complicated path
  f = FileReader("./test_files/../cpplint.py");
  REQUIRE_FALSE(f.read_file(&contents));
}
