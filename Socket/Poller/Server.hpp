#pragma once
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <coroutine>
#include <functional>
#include <future>
#include <iostream>
#include <string>
#include "Socket/ClientSocket.hpp"
#include "Socket/Poller/Helpers.hpp"
#ifdef __APPLE__
#define SOCK_NONBLOCK O_NONBLOCK
#endif

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
  ~SocketBase();

  // Diğer socket işlemleri
  inline bool create(int domain, int type, int protocol);
  inline bool bind();
  // int = -1 ? server : client
  inline bool closeSocket(int);
  [[nodiscard]] inline bool setNonBlocking(bool nonBlocking) const;
  [[nodiscard]] inline bool setReuseAddress(bool reuseAddress) const;
  [[nodiscard]] inline bool setSendTimeout(int milliseconds) const;
  [[nodiscard]] inline bool setRecvTimeout(int milliseconds) const;

  // Polling için yardımcı fonksiyonlar
  [[nodiscard]] inline bool setReadable() const;
  [[nodiscard]] inline bool setWritable() const;

  // while döngüsü ile tüm istekleri kategorize ederek ilgili fonksiyonu çağırır
  // void handler() ;
  inline void handler(HandleOperation&& handleRead,
                      HandleOperation&& handleWrite);
  // Socket isteklerini dinlemek için
  [[nodiscard]] inline bool listen() const;
  [[nodiscard]] int getFd() const;

 private:
  // virtual privates
  CreateSocketT metadata_;
  int socket_ = -1;
  CustomHashedMap clientFds_;
  HandleOperation handleRead_;
  HandleOperation handleWrite_;
  std::mutex clientFdMutex_;
  PollingEvents events_ = {};

  sockaddr_in addr_;
  int addrLen_;
  // Poller Notification Interface
  PollingIdentifier poller_ni_;
};
inline SocketBase::SocketBase(CreateSocketT&& metadata)
    : metadata_(std::move(metadata)) {

  addr_.sin_family = metadata_.domain;
  addr_.sin_addr.s_addr = INADDR_ANY;
  addr_.sin_port = htons(metadata_.port);
  addrLen_ = sizeof(addr_);
  std::cout << "Server Port: " << metadata_.port << std::endl;
  // Create socket
  if (!create(metadata_.domain, metadata_.type, metadata_.protocol)) {
    perror("Socket oluşturma hatası");
    return;
  }
  std::cout << "Socket oluşturuldu: " << socket_ << std::endl;
  // Set socket options
  if (!setReuseAddress(true)) {
    perror("Adresin tekrar kullanılabilir olması ayarlanamadı");
    return;
  }
  std::cout << "Adres tekrar kullanılabilir olarak ayarlandı" << std::endl;
  if (!setNonBlocking(true)) {
    perror("Soketin blok olmayan modda çalışması ayarlanamadı");
    return;
  }
  std::cout << "Soket blok olmayan modda çalışacak şekilde ayarlandı"
            << std::endl;
  // Bind socket
  if (!bind()) {
    perror("Adres ve port bağlama hatası");
    return;
  }
  std::cout << "Adres ve port bağlandı" << std::endl;
  // Listen > if type is SOCK_STREAM and listen is failed
  poller_ni_ = CreatePollingNotificationInterface();
  if (poller_ni_ == -1) {
    perror("Poller oluşturma hatası");
    return;
  }
  std::cout << "Poller oluşturuldu: " << poller_ni_ << std::endl;
  // Soket reader event yapısı
  if (!setPollerReadableWritable(socket_, poller_ni_)) {
    perror("Soket okuma/yazma izinleri ayarlanamadı");
    return;
  }
  std::cout << "Soket okuma/yazma izinleri ayarlandı" << std::endl;

  // clientFds_.reserve(metadata_.backlog);
  events_ = new struct kevent[metadata_.backlog];
  if (metadata_.type == SOCK_STREAM && !listen()) {
    perror("Dinleme hatası");
    return;
  }
  std::cout << "Dinleme başlatıldı" << std::endl;
}
inline SocketBase::~SocketBase() {
  std::cout << "SocketBase Destructed" << std::endl;
  clientFds_.clear();
  close(-1);
  delete[] events_;
  if (poller_ni_ != -1) {
    close(poller_ni_);
  }
}

inline bool SocketBase::create(int domain, int type, int protocol) {
  socket_ = socket(domain, type, protocol);
  if (socket_ < 0) {
    return false;
  }
  return socket_ >= 0;
}

inline bool SocketBase::bind() {
  if (socket_ < 0) {
    return false;
  }

  return ::bind(socket_, reinterpret_cast<struct sockaddr*>(&addr_),
                addrLen_) == 0;
}

inline bool SocketBase::closeSocket(int client) {
  // std::lock_guard<std::mutex> lock(clientFdMutex_);  // Kritik bölgeyi kilitle
  if (!clientFds_.contains(client)) {
    // Göz ardı et
    return true;
  }
  if (client > 0) {
    if (clientFds_[client]->close() < 0) {
      clientFds_.erase(client);
      return false;
    }
    clientFds_.erase(client);
    std::cout << "Connection closed: " << client << std::endl;
    // clientFds_[client] = nullptr;
    return true;
  }
  // Server socket kapat
  socket_ = -1;
  clientFds_.clear();
  exit(EXIT_SUCCESS);
  return ::close(socket_) == 0;
}

inline bool SocketBase::setNonBlocking(bool nonBlocking) const {
  int flags = fcntl(socket_, F_GETFL, 0);
  if (flags == -1) {
    perror("setNonBlocking fcntl");
    return false;
  }
  flags = nonBlocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
  return fcntl(socket_, F_SETFL, flags) == 0;
}

inline bool SocketBase::setReuseAddress(bool reuseAddress) const {
  int option = reuseAddress ? 1 : 0;
  return setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &option,
                    sizeof(option)) == 0;
}

inline bool SocketBase::setSendTimeout(int milliseconds) const {
  struct timeval tv;
  tv.tv_sec = milliseconds / 1000;
  tv.tv_usec = (milliseconds % 1000) * 1000;
  return setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == 0;
}

inline bool SocketBase::setRecvTimeout(int milliseconds) const {
  struct timeval tv;
  tv.tv_sec = milliseconds / 1000;
  tv.tv_usec = (milliseconds % 1000) * 1000;
  return setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0;
}

inline bool SocketBase::listen() const {
  if (socket_ < 0) {
    return false;
  }

  return ::listen(socket_, metadata_.backlog) >= 0;
}

// Platform Spesific Polling (Kqueue, epoll, io_uring)

[[nodiscard]] inline bool SocketBase::setReadable() const {

  return setPollerReadable(socket_, poller_ni_);
}

[[nodiscard]] inline bool SocketBase::setWritable() const {

  return setPollerWritable(socket_, poller_ni_);
}

inline void SocketBase::handler(HandleOperation&& handleRead,
                                HandleOperation&& handleWrite) {
  // auto resp = std::async(std::launch::deferred, [&]() {
  pollingHandler(poller_ni_, socket_, events_, metadata_.backlog, clientFds_,
                 clientFdMutex_, std::move(handleRead), std::move(handleWrite),
                 [&](int fd) { return closeSocket(fd); });
  //   return 0;
  // });
  // resp.wait();
}
[[nodiscard]] inline int SocketBase::getFd() const {
  return socket_;
}
