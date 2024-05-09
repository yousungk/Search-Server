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
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "./FileReader.hpp"
#include "./HttpConnection.hpp"
#include "./HttpRequest.hpp"
#include "./HttpServer.hpp"
#include "./HttpUtils.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::list;
using std::map;
using std::string;
using std::stringstream;
using std::unique_ptr;
using std::vector;

namespace searchserver {
///////////////////////////////////////////////////////////////////////////////
// Constants, internal helper functions
///////////////////////////////////////////////////////////////////////////////
static const char* kFivegleStr =
    "<html><head><title>5950gle</title></head>\n"
    "<body>\n"
    "<center style=\"font-size:500%;\">\n"
    "<span style=\"position:relative;bottom:-0.33em;color:orange;\">5</span>"
    "<span style=\"color:red;\">9</span>"
    "<span style=\"color:gold;\">5</span>"
    "<span style=\"color:blue;\">g</span>"
    "<span style=\"color:green;\">l</span>"
    "<span style=\"color:red;\">e</span>\n"
    "</center>\n"
    "<p>\n"
    "<div style=\"height:20px;\"></div>\n"
    "<center>\n"
    "<form action=\"/query\" method=\"get\">\n"
    "<input type=\"text\" size=30 name=\"terms\" />\n"
    "<input type=\"submit\" value=\"Search\" />\n"
    "</form>\n"
    "</center><p>\n";

// static
const int HttpServer::kNumThreads = 100;

// This is the function that threads are dispatched into
// in order to process new client connections.
static void HttpServer_ThrFn(ThreadPool::Task* t);

// Given a request, produce a response.
static HttpResponse ProcessRequest(const HttpRequest& req,
                                   const string& base_dir,
                                   WordIndex* indices);

// Process a file request.
static HttpResponse ProcessFileRequest(const string& uri,
                                       const string& base_dir);

// Process a query request.
static HttpResponse ProcessQueryRequest(const string& uri, WordIndex* index);

///////////////////////////////////////////////////////////////////////////////
// HttpServer
///////////////////////////////////////////////////////////////////////////////
bool HttpServer::run(void) {
  // Create the server listening socket.
  int listen_fd;
  cout << "  creating and binding the listening socket..." << endl;
  if (!socket_.bind_and_listen(&listen_fd)) {
    cerr << endl << "Couldn't bind to the listening socket." << endl;
    return false;
  }

  // Spin, accepting connections and dispatching them.  Use a
  // threadpool to dispatch connections into their own thread.
  cout << "  accepting connections..." << endl << endl;
  ThreadPool tp(kNumThreads);
  while (1) {
    // create a pointer to a new server task, and pass in the function to
    // execute
    HttpServerTask* hst = new HttpServerTask(HttpServer_ThrFn);
    hst->base_dir = static_file_dir_path_;
    hst->index = index_;
    // fill out information about the client and server inside server task
    // object after accepting client
    if (!socket_.accept_client(&hst->client_fd, &hst->c_addr, &hst->c_port,
                               &hst->c_dns, &hst->s_addr, &hst->s_dns)) {
      // The accept failed for some reason, so quit out of the server.
      // (Will happen when kill command is used to shut down the server.)
      break;
    }
    // The accept succeeded; dispatch it.
    tp.dispatch(hst);
  }
  return true;
}

static void HttpServer_ThrFn(ThreadPool::Task* t) {
  // Cast back our HttpServerTask structure with all of our new
  // client's information in it.
  // given task, cast it to hppt server task
  unique_ptr<HttpServerTask> hst(static_cast<HttpServerTask*>(t));
  cout << "  client " << hst->c_dns << ":" << hst->c_port << " "
       << "(IP address " << hst->c_addr << ")"
       << " connected." << endl;

  // Read in the next request, process it, write the response.

  // Use the HttpConnection class to read and process the next
  // request from our current client, then write out our response.  If
  // the client sends a "Connection: close\r\n" header, then shut down
  // the connection -- we're done.
  //
  // Hint: the client can make multiple requests on our single connection,
  // so we should keep the connection open between requests rather than
  // creating/destroying the same connection repeatedly.

  // TODO: Implement
  bool done = false;
  // create HTTP connection object
  HttpConnection conn(hst->client_fd);
  while (!done) {
    // read in next request from current client
    HttpRequest req;

    // if cannot parse next request, then close connection
    if (!conn.next_request(&req)) {
      cout << "conn.next_request error" << endl;
      done = true;
      break;
    }

    // if client sends close request, then close connection
    if (req.GetHeaderValue("connection") == "close\r\n") {
      cout << "client sent close request" << endl;
      done = true;
      break;
    }

    // process request
    HttpResponse res = ProcessRequest(req, hst->base_dir, hst->index);
    // write response
    conn.write_response(res);
  }
}

static HttpResponse ProcessRequest(const HttpRequest& req,
                                   const string& base_dir,
                                   WordIndex* index) {
  // Is the user asking for a static file?
  if (req.uri().substr(0, 8) == "/static/") {
    return ProcessFileRequest(req.uri(), base_dir);
  }

  // The user must be asking for a query.
  return ProcessQueryRequest(req.uri(), index);
}

static HttpResponse ProcessFileRequest(const string& uri,
                                       const string& base_dir) {
  // The response we'll build up.
  HttpResponse ret;

  // Steps to follow:
  //  - use the URLParser class to figure out what filename
  //    the user is asking for. Note that we identify a request
  //    as a file request if the URI starts with '/static/'
  //
  //  - use the FileReader class to read the file into memory
  //
  //  - copy the file content into the ret.body
  //
  //  - depending on the file name suffix, set the response
  //    Content-type header as appropriate, e.g.,:
  //      --> for ".html" or ".htm", set to "text/html"
  //      --> for ".jpeg" or ".jpg", set to "image/jpeg"
  //      --> for ".png", set to "image/png"
  //      etc.
  //    You should support the file types mentioned above,
  //    as well as ".txt", ".js", ".css", ".xml", ".gif",
  //    and any other extensions to get bikeapalooza
  //    to match the solution server.
  //
  // be sure to set the response code, protocol, and message
  // in the HttpResponse as well.
  string file_name = "";

  // TODO: Implement
  // parse the uri to get the file name
  URLParser parser;
  parser.parse(uri);
  file_name = parser.path();

  // use FileReader to read file into memory
  string content;
  file_name.replace(0, 7, ".");
  FileReader reader(file_name);

  if (reader.read_file(&content)) {
    // set response code, protocol, and message
    ret.set_response_code(200);
    ret.set_protocol("HTTP/1.1");
    ret.set_message("OK");

    // set response content type based on file name suffix
    if (boost::algorithm::ends_with(file_name, ".html") ||
        boost::algorithm::ends_with(file_name, ".htm")) {
      ret.set_content_type("text/html");
    } else if (boost::algorithm::ends_with(file_name, ".jpeg") ||
               boost::algorithm::ends_with(file_name, ".jpg")) {
      ret.set_content_type("image/jpeg");
    } else if (boost::algorithm::ends_with(file_name, ".png")) {
      ret.set_content_type("image/png");
    } else if (boost::algorithm::ends_with(file_name, ".txt")) {
      ret.set_content_type("text/plain");
    } else if (boost::algorithm::ends_with(file_name, ".js")) {
      ret.set_content_type("application/javascript");
    } else if (boost::algorithm::ends_with(file_name, ".css")) {
      ret.set_content_type("text/css");
    } else if (boost::algorithm::ends_with(file_name, ".xml")) {
      ret.set_content_type("application/xml");
    } else if (boost::algorithm::ends_with(file_name, ".gif")) {
      ret.set_content_type("image/gif");
    } else {
      ret.set_content_type("text/plain");
    }

    // copy file into ret.body
    ret.AppendToBody(content);

    // cout << ret.GenerateResponseString() << endl;

    return ret;
  } else {
    // If you couldn't find the file, return an HTTP 404 error.
    ret.set_protocol("HTTP/1.1");
    ret.set_response_code(404);
    ret.set_message("Not Found");
    ret.AppendToBody("<html><body>Couldn't find file \"" +
                     escape_html(file_name) + "\"</body></html>\n");

    // cout << ret.GenerateResponseString() << endl;

    return ret;
  }
}

static HttpResponse ProcessQueryRequest(const string& uri, WordIndex* index) {
  // The response we're building up.
  HttpResponse ret;

  // Your job here is to figure out how to present the user with
  // the same query interface as our solution_binaries/httpd server.
  // A couple of notes:
  //
  //  - no matter what, you need to present the 5950gle logo and the
  //    search box/button
  //
  //  - if the user sent in a search query, you also
  //    need to display the search results. You can run the solution binaries to
  //    see how these should look
  //
  //  - you'll want to use the URLParser to parse the uri and extract
  //    search terms from a typed-in search query.  convert them
  //    to lower case.
  //
  //  - Use the specified index to generate the query results

  // TODO: implement
  // parse the uri to get the search terms
  // convert to lower case

  URLParser parser;
  parser.parse(uri);
  map<string, string> args = parser.args();
  vector<string> terms;
  bool term_exists = (args.find("terms") != args.end());
  boost::split(terms, args["terms"], boost::is_any_of("+ "),
               boost::token_compress_on);
  for (auto& term : terms) {
    boost::to_lower(term);
    boost::trim(term);
  }
  terms.erase(std::remove(terms.begin(), terms.end(), ""), terms.end());

  // return results
  // also return the logo and search box/button
  ret.set_response_code(200);
  ret.set_protocol("HTTP/1.1");
  ret.set_message("OK");
  ret.set_content_type("text/html");

  stringstream ss;
  ss << kFivegleStr;
  if (term_exists) {
    // user index to generate query results
    vector<Result> results = index->lookup_query(terms);
    boost::to_lower(args["terms"]);
    if (terms.size() == 0 || results.size() == 0) {
      ss << "<p><br>\n";
      ss << "No results found for <b>" << args["terms"] << "</b>\n";
      ss << "<p>\n\n";
    } else {
      ss << "<p><br>\n";
      if (results.size() == 1) {
        ss << "1 result found for <b>" << args["terms"] << "</b>\n";
      } else {
        ss << results.size() << " results found for <b>" << args["terms"]
           << "</b>\n";
      }
      ss << "<p>\n\n";

      ss << "<ul>\n";
      for (auto& result : results) {
        ss << " <li> <a href=\"/static/" << result.doc_name << "\">"
           << result.doc_name << "</a> [" << result.rank << "]<br>\n";
      }
      ss << "</ul>\n";
    }
  }
  ss << "</body>\n";
  ss << "</html>\n";
  ret.AppendToBody(ss.str());

  // cout << ret.GenerateResponseString() << endl;

  return ret;
}

}  // namespace searchserver
