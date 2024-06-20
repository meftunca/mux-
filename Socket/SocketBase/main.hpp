#pragma once
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <coroutine>
#include <functional>
#include <future>
#include <iostream>
#include <string>
#include "Socket/ClientSocket.hpp"
#ifdef __APPLE__
#define SOCK_NONBLOCK O_NONBLOCK
#endif
using HandleOperation =
    std::function<int(std::shared_ptr<ClientSocket>)>;  // (client) -> int

struct CreateSocketT {
  int domain = AF_INET;
  int type = SOCK_STREAM;
  int protocol = IPPROTO_TCP;  // 0 = TCP, IPPROTO_TCP
  const char* address = "127.0.0.1";
  int port = 3732;
  const int backlog = 1000 * 64;  //  queue limit for incoming connections
  int default_buffer_size = 1024 * 4;
  bool* ipv6 = nullptr;
};
class SocketBase {
 public:
  explicit SocketBase(CreateSocketT&&);
  SocketBase() = default;
  ~SocketBase() = default;

  // Diğer socket işlemleri
  virtual inline bool create(int domain, int type, int protocol) = 0;
  virtual inline bool bind() = 0;
  // int = -1 ? server : client
  virtual inline bool closeSocket(int) = 0;
  virtual inline bool setNonBlocking(bool nonBlocking) = 0;
  virtual inline bool setReuseAddress(bool reuseAddress) = 0;
  virtual inline bool setSendTimeout(int milliseconds) = 0;
  virtual inline bool setRecvTimeout(int milliseconds) = 0;

  // Polling için yardımcı fonksiyonlar
  [[nodiscard]] virtual inline bool setReadable() const = 0;
  [[nodiscard]] virtual inline bool setWritable() const = 0;

  // while döngüsü ile tüm istekleri kategorize ederek ilgili fonksiyonu çağırır
  // virtual void handler() = 0;
  virtual inline void handler(HandleOperation&& handleRead,
                              HandleOperation&& handleWrite) = 0;
  // Socket isteklerini dinlemek için
  virtual inline bool listen() = 0;
  [[nodiscard]] virtual int getFd() const = 0;

 private:
  CreateSocketT metadata_;
  std::unordered_map<int, std::shared_ptr<ClientSocket>> clientFds_;
  int socket_ = -1;
  HandleOperation handleRead_;
  HandleOperation handleWrite_;
  sockaddr_in addr_;
  int addrLen_;
};
