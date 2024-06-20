#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <coroutine>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#ifdef __linux__
#ifndef EPOLL_ENABLE  // eğer tanımlı değilse epoll yerine iouring kullan
#define EPOLL_ENABLE 0
#include <liburing.h>
using DEFAULT_POOLER_TYPE = struct io_uring*;
#else
#include <sys/epoll.h>
using DEFAULT_POOLER_TYPE = int;
#endif

#elif __APPLE__
using DEFAULT_POOLER_TYPE = int;

#endif

// C++ 20 ise

// Aligned memory allocation
class ClientSocket {
 public:
  ClientSocket(DEFAULT_POOLER_TYPE poller, int server_socket)
      : poller_(poller), server_socket_(server_socket) {}
  ~ClientSocket() {
    std::cout << "ClientSocket Destructor\n";
    if (isConnected_) {
      close();
    }
  };
  ClientSocket(ClientSocket&& other) noexcept
      : poller_(other.poller_), server_socket_(other.server_socket_) {}
  inline int accept();
  inline int read(char*, size_t);
  inline int write(std::string& buff);
  inline int close();
  inline int setReader();
  inline int setWriter();
  inline int removeReader();
  inline int removeWriter();

  [[nodiscard]] inline int getFd() const { return clientFd_; }

 private:
  DEFAULT_POOLER_TYPE poller_;
  int server_socket_;
  int clientFd_ = -1;
  bool isReader_ = false;
  bool isWriter_ = false;
  bool isConnected_ = false;
};

// Safety close
inline int safetyClose(int clientSocket) {

  if (shutdown(clientSocket, SHUT_RDWR) < 0) {
    perror("Shutdown failed");
    return -1;
  }
  return ::close(clientSocket);
}

inline int ClientSocket::read(char* buff, size_t size) {
  if (!isConnected_ || !isReader_ || setReader() == -1) {
    return -1;
  }
  isConnected_ = true;
  int bytesRead = ::recv(clientFd_, buff, size, 0);
  if (bytesRead == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    }
    perror("read");
    return -1;
  }
  return bytesRead;
}

inline int ClientSocket::write(std::string& buff) {
  if (!isConnected_ || !isWriter_ || setWriter() == -1) {
    return -1;
  }
  int bytesWritten = ::send(clientFd_, buff.data(), buff.size(), 0);
  if (bytesWritten == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    }
    perror("write");
    return -1;
  }
  return bytesWritten;
}

inline int defaultAcception(int server_socket_) {
  const int clientFd =
      ::accept(server_socket_, nullptr,
               nullptr);  // (struct sockaddr *)&client_addr, &client_len);
  if (clientFd < 0) {
    perror("accept");
    return -1;
  }
  // Set NonBlocking
  int flags = fcntl(clientFd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl");
    return safetyClose(clientFd);
  }
  if (!(flags & O_RDWR)) {  // Soket açık değilse
    perror("Socket is not open");
    return -1;  // Veya hata durumunu uygun şekilde ele alın
  }
  flags = flags | O_NONBLOCK;
  if (fcntl(clientFd, F_SETFL, flags) == -1) {
    perror("fcntl");
    return safetyClose(clientFd);
  }
  int flag = 1;
  if (setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY,
                 reinterpret_cast<char*>(&flag), sizeof(int)) == -1) {
    perror("setsockopt");
    return safetyClose(clientFd);
  }

  return clientFd;
}

inline int ClientSocket::accept() {
  // struct sockaddr_in client_addr;
  // socklen_t client_len = sizeof(client_addr);
  auto clientFD = defaultAcception(server_socket_);
  if (clientFD == -1) {
    return -1;
  }
  isConnected_ = true;
  clientFd_ = clientFD;
  // Set Reader & Writer
  if (setReader() == -1) {
    perror("setClientReader");
    safetyClose(clientFD);
    return -1;
  }
  if (setWriter() == -1) {
    perror("setClientWriter");
    safetyClose(clientFD);
    return -1;
  }
  isConnected_ = true;
  return clientFD;
}
// Close client socket
inline int ClientSocket::close() {
  if (!isConnected_ || clientFd_ < 0) {
    return 1;
  }
  if (removeReader() < 0 || removeWriter() < 0) {
    perror("Client Read/write izinleri kaldırılamadı");
  }
  isConnected_ = false;
  // Close socket
  return safetyClose(clientFd_);
}

inline int ClientSocket::setReader() {
  if (isReader_) {
    return 1;
  }

  struct kevent changes;
  EV_SET(&changes, clientFd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
  if (kevent(poller_, &changes, 1, nullptr, 0, nullptr) == -1) {
    perror("kevent");
    return -1;
  }
  isReader_ = true;
  return 1;
}

inline int ClientSocket::removeReader() {
  if (!isReader_) {
    return 1;
  }
  struct kevent changes;
  EV_SET(&changes, clientFd_, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
  if (kevent(poller_, &changes, 1, nullptr, 0, nullptr) == -1) {
    perror("kevent");
    return -1;
  }
  isReader_ = false;
  return 1;
}

inline int ClientSocket::setWriter() {
  if (isWriter_) {
    return 1;
  }
  struct kevent changes;
  EV_SET(&changes, clientFd_, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, nullptr);
  if (kevent(poller_, &changes, 1, nullptr, 0, nullptr) == -1) {
    perror("kevent");
    return -1;
  }
  isWriter_ = true;
  return 1;
}

inline int ClientSocket::removeWriter() {
  if (!isWriter_) {
    return 1;
  }
  struct kevent changes;
  EV_SET(&changes, clientFd_, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
  if (kevent(poller_, &changes, 1, nullptr, 0, nullptr) == -1) {
    perror("kevent");
    return -1;
  }
  isWriter_ = false;
  return 1;
}