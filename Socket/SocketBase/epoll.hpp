#ifdef __linux__

#include <sys/epoll.h>
#include <thread>
#include <unordered_map>
#include "Socket/ClientSocket.hpp"
#include "Socket/SocketBase/main.hpp"

class EpollSocket : public SocketBase {
 public:
  explicit EpollSocket(CreateSocketT&& metadata)
      : metadata_(std::move(metadata)) {
    epoll_fd_ = epoll_create1(0);
    addr_.sin_family = metadata_.domain;
    addr_.sin_addr.s_addr = INADDR_ANY;
    addr_.sin_port = htons(metadata_.port);
    addrLen_ = sizeof(addr_);

    if (epoll_fd_ == -1) {
      perror("epoll_create1 hatası");
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

    // epoll reader event yapısı
    if (!setReadable()) {
      perror("epoll okunabilirlik ayarlanamadı");
      return;
    }

    // epoll writer event yapısı
    if (!setWritable()) {
      perror("epoll yazılabilirlik ayarlanamadı");
      return;
    }

    events_ = new struct epoll_event[metadata_.backlog];
  }
  EpollSocket() = default;
  ~EpollSocket() {
    std::perror("EV_EOF EpollSocket destroyed: ");
    clientFds_.clear();
    close(-1);
    if (epoll_fd_ != -1) {
      close(epoll_fd_);
    }
    delete[] events_;
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
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = socket_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_, &event) != 0) {
      perror("epoll_ctl ADD hatası (EPOLLIN)");
      return false;
    }
    return true;
  }

  [[nodiscard]] inline bool setWritable() const override {
    struct epoll_event event;
    event.events = EPOLLOUT;
    event.data.fd = socket_;
    return epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_, &event) == 0;
  }

  inline bool listen() override {
    if (socket_ < 0) {
      return false;
    }

    return ::listen(socket_, metadata_.backlog) >= 0;
  }

  inline void handler(HandleOperation&& handleRead,
                      HandleOperation&& handleWrite) override {
    int numEvents = epoll_wait(epoll_fd_, events_, metadata_.backlog, -1);
    if (numEvents == -1) {
      perror("epoll_wait");
      return;
    }

    for (int i = 0; i < numEvents; ++i) {
      const auto event = events_[i];
      const int fd = event.data.fd;

      // handle close
      if ((event.events & EPOLLHUP) || (event.events & EPOLLERR)) {
        if (fd != socket_) {
          if (!closeSocket(fd)) {
            perror("Socket Kapatılıyor, çünkü beklenmedik bir hata oluştu:");
          }
          continue;
        }
        perror("EPOLLHUP EpollSocket handler: ");
        closeSocket(-1);
        return;
      }
      if (fd == socket_) {
        std::unique_lock<std::mutex> lock(
            clientFdMutex_);  // Kritik bölgeyi kilitle
        auto clientSock = std::make_shared<ClientSocket>(epoll_fd_, socket_);

        auto clientFD = clientSock->accept();
        if (clientFD == -1) {
          perror("accept");
          continue;
        }
        // Yeni istemci bağlantısı
        clientFds_[clientFD] = clientSock;
        continue;
      }
      const auto clientSock = clientFds_[fd];

      if (event.events & EPOLLIN) {
        // Okuma olayı (paralel olarak işlenecek)
        if (handleRead(clientSock) < 0) {
          perror("handleRead_");
          closeSocket(fd);
          continue;
        }
        // Okuma işlemi başarılı ise yazma işlemi için event ekle
        clientSock->setWriter();
        continue;
      }
      if (event.events & EPOLLOUT) {
        // Yazma olayı (paralel olarak işlenecek)
        if (handleWrite(clientSock) <= 0) {
          perror("handleWrite");
          closeSocket(fd);
        }
        // Yazma işlemi tamamlandı, yazma işlemini kaldır
        clientSock->removeWriter();
        continue;
      }
      // Diğer olaylar
      std::cerr << "Diğer olaylar: " << event.events << std::endl;
      closeSocket(fd);
      perror("Socket anlaşılmadık bir şekilde kapandı: ");
    }
  }

  [[nodiscard]] inline int getFd() const override { return socket_; }

 private:
  CreateSocketT metadata_;
  int socket_ = -1;
  std::unordered_map<int, std::shared_ptr<ClientSocket>, IntHash> clientFds_;
  HandleOperation handleRead_;
  HandleOperation handleWrite_;
  std::mutex clientFdMutex_;
  struct epoll_event* events_ = {};

  sockaddr_in addr_;
  int addrLen_;
  int epoll_fd_;
};

#endif
