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

#include <arpa/inet.h>   // for inet_ntop()
#include <errno.h>       // for errno, used by strerror()
#include <netdb.h>       // for getaddrinfo()
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <unistd.h>      // for close(), fcntl()
#include <cstdio>        // for snprintf()
#include <cstring>       // for memset, strerror()
#include <iostream>      // for std::cerr, etc.

#include "./ServerSocket.hpp"

namespace searchserver {

ServerSocket::ServerSocket(uint16_t port) : port_(port), listen_sock_fd_(-1) {}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1) {
    close(listen_sock_fd_);
  }
  listen_sock_fd_ = -1;
}

bool ServerSocket::bind_and_listen(int* listen_fd) {
  // Use "getaddrinfo," "socket," "bind," and "listen" to
  // create a listening socket on port port_.  Return the
  // listening socket through the output parameter "listen_fd"
  // and set the ServerSocket data member "listen_sock_fd_"

  // NOTE: You only have to support IPv6, you do not have to support IPv4

  // TODO: implement
  struct addrinfo hints = {};
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo* result = nullptr;

  if (getaddrinfo(nullptr, std::to_string(port_).c_str(), &hints, &result) !=
      0) {
    return false;
  }

  int socket_fd = -1;
  for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
    socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (socket_fd == -1) {
      continue;
    }

    int optval = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                   sizeof(optval)) == 0) {
      if (bind(socket_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
        break;
      }
    }

    close(socket_fd);
    socket_fd = -1;
  }

  freeaddrinfo(result);

  if (socket_fd == -1) {
    listen_sock_fd_ = -1;
    *listen_fd = -1;
    return false;
  }

  if (listen(socket_fd, SOMAXCONN) != 0) {
    close(socket_fd);
    listen_sock_fd_ = -1;
    *listen_fd = -1;
    return false;
  }

  listen_sock_fd_ = socket_fd;
  *listen_fd = socket_fd;

  return true;
}

bool ServerSocket::accept_client(int* accepted_fd,
                                 std::string* client_addr,
                                 uint16_t* client_port,
                                 std::string* client_dns_name,
                                 std::string* server_addr,
                                 std::string* server_dns_name) const {
  // Accept a new connection on the listening socket listen_sock_fd_.
  // (Block until a new connection arrives.)  Return the newly accepted
  // socket, as well as information about both ends of the new connection,
  // through the various output parameters.

  // TODO: implement
  while (1) {
    struct sockaddr_storage caddr;
    socklen_t caddr_len = sizeof(caddr);
    struct sockaddr* addr = reinterpret_cast<struct sockaddr*>(&caddr);
    int client_fd = accept(listen_sock_fd_, addr, &caddr_len);

    if (client_fd < 0) {
      if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        continue;
      }
      return false;
    }

    *accepted_fd = client_fd;

    // client_addr and client_port
    char astring[INET6_ADDRSTRLEN];
    struct sockaddr_in6* in6 = reinterpret_cast<struct sockaddr_in6*>(addr);
    inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
    *client_addr = astring;
    *client_port = ntohs(in6->sin6_port);

    // client_dns_name
    char hostname[1024];
    getnameinfo(addr, caddr_len, hostname, 1024, nullptr, 0, 0);
    *client_dns_name = hostname;

    // server_addr and server_dns_name
    struct sockaddr_in6 srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET6_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr*)&srvr, &srvrlen);
    inet_ntop(AF_INET6, &srvr.sin6_addr, addrbuf, INET6_ADDRSTRLEN);
    char hname[1024] = {};
    getnameinfo((const struct sockaddr*)&srvr, srvrlen, hname, 1024, nullptr, 0,
                0);
    *server_addr = addrbuf;
    *server_dns_name = hname;

    return true;
  }
  return false;
}

}  // namespace searchserver
