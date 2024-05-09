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

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "./HttpConnection.hpp"
#include "./HttpRequest.hpp"
#include "./HttpUtils.hpp"

using std::map;
using std::string;
using std::vector;

namespace searchserver {

static const char* kHeaderEnd = "\r\n\r\n";
static const int kHeaderEndLen = 4;

bool HttpConnection::next_request(HttpRequest* request) {
  // Use "wrapped_read" to read data into the buffer_
  // instance variable.  Keep reading data until either the
  // connection drops or you see a "\r\n\r\n" that demarcates
  // the end of the request header.
  //
  // Once you've seen the request header, use parse_request()
  // to parse the header into the *request argument.
  //
  // Very tricky part:  clients can send back-to-back requests
  // on the same socket.  So, you need to preserve everything
  // after the "\r\n\r\n" in buffer_ for the next time the
  // caller invokes next_request()!

  // TODO: implement

  size_t pos = buffer_.find(kHeaderEnd);
  while (pos == string::npos) {
    wrapped_read(fd_, &buffer_);
    pos = buffer_.find(kHeaderEnd);
  }

  string req_str = buffer_.substr(0, pos);
  buffer_.erase(0, pos + kHeaderEndLen);

  return parse_request(req_str, request);
}

bool HttpConnection::write_response(const HttpResponse& response) {
  // Implement so that the response is converted to a string
  // and written out to the socket for this connection

  // TODO: implement
  string res = response.GenerateResponseString();
  write(fd_, res.c_str(), res.size());

  return true;
}

bool HttpConnection::parse_request(const string& request, HttpRequest* out) {
  HttpRequest req("/");  // by default, get "/".

  // Split the request into lines.  Extract the URI from the first line
  // and store it in req.URI.  For each additional line beyond the
  // first, extract out the header name and value and store them in
  // req.headers_ (i.e., HttpRequest::AddHeader).  You should look
  // at HttpRequest.h for details about the HTTP header format that
  // you need to parse.
  //
  // You'll probably want to look up boost functions for (a) splitting
  // a string into lines on a "\r\n" delimiter, (b) trimming
  // whitespace from the end of a string, and (c) converting a string
  // to lowercase.
  //
  // If a request is malfrormed, return false, otherwise true and
  // the parsed request is retrned via *out

  // TODO: implement
  vector<string> lines;
  boost::split(lines, request, boost::is_any_of("\r\n"));
  lines.erase(std::remove_if(lines.begin(), lines.end(),
                             [](const std::string& s) { return s.empty(); }),
              lines.end());

  vector<string> first_line_tokens;
  boost::split(first_line_tokens, lines.at(0), boost::is_any_of(" "));
  out->set_uri(first_line_tokens.at(1));
  lines.erase(lines.begin());

  for (string& line : lines) {
    vector<string> tokens;
    string::size_type pos = line.find(":");

    if (pos == string::npos) {
      return false;
    }

    string key = line.substr(0, pos);
    boost::trim(key);
    boost::algorithm::to_lower(key);

    string value = line.substr(pos + 1);
    boost::trim(value);
    boost::algorithm::to_lower(value);

    out->AddHeader(key, value);
  }

  return true;
}

}  // namespace searchserver
