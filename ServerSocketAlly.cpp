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
#include <array>

#include "./ServerSocket.hpp"

namespace searchserver {

ServerSocket::ServerSocket(uint16_t port) {
  port_ = port;
  listen_sock_fd_ = -1;
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

bool ServerSocket::bind_and_listen(int* listen_fd) {
  // Use "getaddrinfo," "socket," "bind," and "listen" to
  // create a listening socket on port port_.  Return the
  // listening socket through the output parameter "listen_fd"
  // and set the ServerSocket data member "listen_sock_fd_"

  // NOTE: You only have to support IPv6, you do not have to support IPv4

  // TODO: implement

  // get my address information
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET6;       // IPv6 (also handles IPv4 clients)
  hints.ai_socktype = SOCK_STREAM;  // stream
  hints.ai_flags = AI_PASSIVE;   // use an address we can bind to a socket and
                                 // accept client connections on
  hints.ai_flags = AI_V4MAPPED;  // use v4-mapped v6 if no v6 found
  hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;

  struct addrinfo* result = nullptr;
  int res =
      getaddrinfo(nullptr, std::to_string(port_).c_str(), &hints, &result);

  if (res != 0) {
    std::cerr << "getaddrinfo failed: " << gai_strerror(res) << std::endl;
    return false;
  }

  // loop through the results and bind to the first one that works
  for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
    *listen_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

    if (*listen_fd == -1) {
      listen_fd = 0;
      continue;
    } else {
      int optval = 1;
      setsockopt(*listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
      // bind socket to the address and port
      if (bind(*listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
        sock_family_ = rp->ai_family;
        break;
      } else {
        close(*listen_fd);
        *listen_fd = -1;
      }
    }
  }

  // free the address info
  freeaddrinfo(result);

  // if we didn't bind to a socket, return false
  if (*listen_fd == -1) {
    return false;
  }

  // listen on the socket
  if (listen(*listen_fd, SOMAXCONN) != 0) {
    close(*listen_fd);
    *listen_fd = -1;
    return false;
  }

  // return listening socket file descriptor through listen_fd
  listen_sock_fd_ = *listen_fd;
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

  // accept new client
  // once accept, return information about the client and server socket
  while (1) {
    struct sockaddr_storage caddr;
    socklen_t client_addr_len = sizeof(caddr);
    int client_fd = accept(listen_sock_fd_,
                           reinterpret_cast<struct sockaddr*>(&caddr),
                           &client_addr_len);

    if (client_fd < 0) {
      if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        continue;
      }
      std::cerr << "Failure on accept: " << strerror(errno) << std::endl;
      break;
    } else {
      // set accepted client fd
      *accepted_fd = client_fd;
      // set server address
      // set server dns name
      // set client address
      // set client dns name
      // set client port
      std::array<char, 1024> hname;
      if (sock_family_ == AF_INET) {
        struct sockaddr_in server;
        socklen_t server_len = sizeof(server);
        std::array<char, INET_ADDRSTRLEN> addrbuf;

        getsockname(client_fd, (struct sockaddr*)&server, &server_len);
        inet_ntop(AF_INET, &(server.sin_addr), addrbuf.data(), INET_ADDRSTRLEN);

        // set server address
        *server_addr = addrbuf.data();
        
        // set dns name
        getnameinfo((struct sockaddr*)&server, server_len, hname.data(), 1024,
                    nullptr, 0, 0);
        *server_dns_name = hname.data();

        std::array<char, INET_ADDRSTRLEN> client_addr_buf;
        struct sockaddr_in* client =
            reinterpret_cast<struct sockaddr_in*>(&client_addr);
        socklen_t client_len = sizeof(*client);
        // set client address
        inet_ntop(AF_INET, &(client->sin_addr), client_addr_buf.data(),
                  INET_ADDRSTRLEN);
        *client_addr = client_addr_buf.data();
        // set client port
        *client_port = ntohs(client->sin_port);
        // set client dns name
        getnameinfo((struct sockaddr*)client, client_len, client_dns_name->data(), 1024,
                    nullptr, 0, 0);
      } else {
        struct sockaddr_in server;
        socklen_t server_len = sizeof(server);
        std::array<char, INET6_ADDRSTRLEN> addrbuf;
        getsockname(client_fd, (struct sockaddr*)&server, &server_len);
        inet_ntop(AF_INET6, &server.sin_addr, addrbuf.data(), INET6_ADDRSTRLEN);
        // set server address
        *server_addr = addrbuf.data();
        // set dns name
        getnameinfo((struct sockaddr*)&server, server_len, hname.data(), 1024,
                    nullptr, 0, 0);
        *server_dns_name = hname.data();
      }
    }
  }
  return true;
}

}  // namespace searchserver
