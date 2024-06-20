#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include "Socket/ClientSocket.hpp"
#include "Socket/SocketBase/main.hpp"
struct IntHash {
  std::size_t operator()(int key) const {
    return static_cast<std::size_t>(key);
  }
};

struct IntHashBitRotate {
  std::size_t operator()(int key) const {
    return std::rotl(static_cast<std::size_t>(key), 5);  // 5 bit döndürme
  }
};
class KqueueSocket : public SocketBase {
 public:
  explicit KqueueSocket(CreateSocketT&& metadata)
      : metadata_(std::move(metadata)) {
    kq_ = kqueue();
    addr_.sin_family = metadata_.domain;
    addr_.sin_addr.s_addr = INADDR_ANY;
    addr_.sin_port = htons(metadata_.port);
    addrLen_ = sizeof(addr_);

    if (kq_ == -1) {
      perror("Kqueue oluşturma hatası");
      return;
    }
    // Create socket
    if (!create(metadata_.domain, metadata_.type, metadata_.protocol)) {
      perror("Socket oluşturma hatası");
      return;
    }
    // Set socket options
    if (!setReuseAddress(true)) {
      perror("Adresin tekrar kullanılabilir olması ayarlanamadı");
      return;
    }
    if (!setNonBlocking(true)) {
      perror("Soketin blok olmayan modda çalışması ayarlanamadı");
      return;
    }
    // Bind socket
    if (!bind()) {
      perror("Adres ve port bağlama hatası");
      return;
    }
    // Listen > if type is SOCK_STREAM and listen is failed
    if (metadata_.type == SOCK_STREAM && !listen()) {
      std::cerr << "Listen hatası" << strerror(errno) << std::endl;
      return;
    }

    // Kqueue reader event yapısı
    if (!setReadable()) {
      perror("Kqueue okunabilirlik ayarlanamadı");
      return;
    }

    // Kqueue writer event yapısı
    if (!setWritable()) {
      perror("Kqueue yazılabilirlik ayarlanamadı");
      return;
    }

    // clientFds_.reserve(metadata_.backlog);
    events_ = new struct kevent[metadata_.backlog];
  }
  KqueueSocket() = default;
  ~KqueueSocket() {
    std::perror("EV_EOF KqueueSocket destroyed: ");
    clientFds_.clear();
    close(-1);
    delete[] events_;
    if (kq_ != -1) {
      close(kq_);
    }
  }

  inline bool create(int domain, int type, int protocol) override {
    socket_ = socket(domain, type, protocol);
    if (socket_ < 0) {
      return false;
    }
    return socket_ >= 0;
  }

  inline bool bind() override {
    if (socket_ < 0) {
      return false;
    }

    return ::bind(socket_, reinterpret_cast<struct sockaddr*>(&addr_),
                  addrLen_) == 0;
  }

  inline bool closeSocket(int client) override {
    if (!clientFds_.contains(client)) {
      // Göz ardı et
      return true;
    }
    if (client >= 0) {
      std::unique_lock<std::mutex> lock(
          clientFdMutex_);  // Kritik bölgeyi kilitle
      if (clientFds_[client]->close() == -1) {
        perror("Client Socket Not Closed: ");
        return false;
      }
      clientFds_.erase(client);
      // clientFds_[client] = nullptr;
      return true;
    }
    // Server socket kapat
    socket_ = -1;
    clientFds_.clear();

    return ::close(socket_) == 0;
  }

  inline bool setNonBlocking(bool nonBlocking) override {
    int flags = fcntl(socket_, F_GETFL, 0);
    if (flags == -1) {
      perror("setNonBlocking fcntl");
      return false;
    }
    flags = nonBlocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return fcntl(socket_, F_SETFL, flags) == 0;
  }

  inline bool setReuseAddress(bool reuseAddress) override {
    int option = reuseAddress ? 1 : 0;
    return setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &option,
                      sizeof(option)) == 0;
  }

  inline bool setSendTimeout(int milliseconds) override {
    struct timeval tv;
    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    return setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == 0;
  }

  inline bool setRecvTimeout(int milliseconds) override {
    struct timeval tv;
    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    return setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0;
  }

  [[nodiscard]] inline bool setReadable() const override {

    struct kevent event;
    EV_SET(&event, socket_, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0,
           nullptr);
    return kevent(kq_, &event, 1, &event, 1, nullptr) == 1 &&
           (event.flags & EV_ERROR) == 0;
  }

  [[nodiscard]] inline bool setWritable() const override {

    struct kevent event;
    EV_SET(&event, socket_, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0,
           nullptr);
    return kevent(kq_, &event, 1, &event, 1, nullptr) == 1 &&
           (event.flags & EV_ERROR) == 0;
  }

  inline bool listen() override {
    if (socket_ < 0) {
      return false;
    }

    return ::listen(socket_, metadata_.backlog) >= 0;
  }

  inline void handler(HandleOperation&& handleRead,
                      HandleOperation&& handleWrite) override {
    int numEvents =
        kevent(kq_, nullptr, 0, events_, metadata_.backlog, nullptr);
    if (numEvents == -1) {
      perror("kevent(numEvents)");
      return;
    }

    for (int i = 0; i < numEvents; ++i) {
      const auto event = events_[i];
      const int fd = event.ident;
      const int filter = event.filter;

      // handle close
      if (event.flags & EV_EOF && fd != socket_) {

        if (fd != socket_) {
          if (!closeSocket(fd)) {
            perror("Socket Kapatılıyor, çünkü beklenmedik bir hata oluştu:");
          }
          continue;
        }
        perror("EV_EOF KqueueSocket handler: ");
        closeSocket(-1);
        return;
      }
      if (fd == socket_) {
        std::unique_lock<std::mutex> lock(
            clientFdMutex_);  // Kritik bölgeyi kilitle
        auto clientSock = std::make_shared<ClientSocket>(kq_, socket_);
        // auto clientSock = std::shared_ptr<ClientSocket>(kq_, socket_);

        auto clientFD = clientSock->accept();
        if (clientFD == -1) {
          perror("accept");
          continue;
        }
        // Yeni istemci bağlantısı
        clientFds_[clientFD] = clientSock;
        // clientFds_.emplace(clientFD, clientSock);
        // clientFds_.insert({clientFD, clientSock});
        continue;
      }
      const auto clientSock = clientFds_[fd];

      // process ?
      if (filter == EVFILT_READ) {
        // Okuma olayı (paralel olarak işlenecek)
        if (handleRead(clientSock) < 0) {
          perror("handleRead_");
          closeSocket(fd);
          continue;
        }
        // Okuma işlemi başarılı ise yazma işlemi için event ekle
        clientSock->setWriter();
        continue;
        //
      }
      if (filter == EVFILT_WRITE) {
        // Yazma olayı (paralel olarak işlenecek)
        if (handleWrite(clientSock) <= 0) {
          perror("handleWrite");
          closeSocket(fd);
        }
        // Yazma işlemi tamamlandı, yazma işlemini kaldır
        // removeClientWriter(kq_, fd);
        clientSock->removeWriter();  //  yazma işlemini kaldır
        continue;
      }
      // Diğer olaylar
      std::cerr << "Diğer olaylar: " << event.filter << std::endl;
      closeSocket(fd);
      perror("Socket anlaşılmadık bir şekilde kapandı: ");
    }
  }
  [[nodiscard]] inline int getFd() const override { return socket_; }

 private:
  // virtual privates
  CreateSocketT metadata_;
  int socket_ = -1;
  std::unordered_map<int, std::shared_ptr<ClientSocket>, IntHash> clientFds_;
  HandleOperation handleRead_;
  HandleOperation handleWrite_;
  std::mutex clientFdMutex_;
  struct kevent* events_ = {};

  // struct kevent mainEvent_;
  sockaddr_in addr_;
  int addrLen_;
  // default timeout
  // struct timespec timeout_ = {0, 50};
  // macos, freebsd structures
  int kq_;
};
