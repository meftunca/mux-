#ifdef __linux__

#include <liburing.h>
#include <thread>
#include <unordered_map>
#include "Socket/ClientSocket.hpp"
#include "Socket/SocketBase/main.hpp"

class IoUringSocket : public SocketBase {
 public:
  explicit IoUringSocket(CreateSocketT&& metadata)
      : metadata_(std::move(metadata)) {
    if (io_uring_queue_init(32, &ring_, 0) < 0) {
      throw std::runtime_error("io_uring_queue_init hatası");
    }
    addr_.sin_family = metadata_.domain;
    addr_.sin_addr.s_addr = INADDR_ANY;
    addr_.sin_port = htons(metadata_.port);
    addrLen_ = sizeof(addr_);

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

    events_ = new io_uring_cqe*[metadata_.backlog];
  }
  IoUringSocket() = default;
  ~IoUringSocket() {
    std::perror("EV_EOF IoUringSocket destroyed: ");
    clientFds_.clear();
    close(-1);
    io_uring_queue_exit(&ring_);
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
    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
      perror("io_uring_get_sqe hatası (POLLIN)");
      return false;
    }
    io_uring_prep_poll_add(sqe, socket_, POLLIN);
    return io_uring_submit(&ring_) > 0;
  }

  [[nodiscard]] inline bool setWritable() const override {
    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    io_uring_prep_poll_add(sqe, socket_, POLLOUT);
    return io_uring_submit(&ring_) > 0;
  }

  inline bool listen() override {
    if (socket_ < 0) {
      return false;
    }

    return ::listen(socket_, metadata_.backlog) >= 0;
  }

  inline void handler(HandleOperation&& handleRead,
                      HandleOperation&& handleWrite) override {
    int numEvents;
    io_uring_cqe* cqe;

    while ((numEvents = io_uring_wait_cqe(&ring_, &cqe)) >= 0) {
      const int fd = cqe->fd;

      // handle close
      if (cqe->res < 0) {
        if (fd != socket_) {
          if (!closeSocket(fd)) {
            perror("Socket Kapatılıyor, çünkü beklenmedik bir hata oluştu:");
          }
          io_uring_cqe_seen(&ring_, cqe);
          continue;
        }
        perror("IO_URING EV_EOF IoUringSocket handler: ");
        closeSocket(-1);
        io_uring_cqe_seen(&ring_, cqe);
        return;
      }
      if (fd == socket_) {
        std::unique_lock<std::mutex> lock(
            clientFdMutex_);  // Kritik bölgeyi kilitle
        auto clientSock = std::make_shared<ClientSocket>(ring_, socket_);

        auto clientFD = clientSock->accept();
        if (clientFD == -1) {
          perror("accept");
          io_uring_cqe_seen(&ring_, cqe);
          continue;
        }
        // Yeni istemci bağlantısı
        clientFds_[clientFD] = clientSock;
        io_uring_cqe_seen(&ring_, cqe);
        continue;
      }
      const auto clientSock = clientFds_[fd];

      if (cqe->flags & IORING_CQE_F_MORE) {
        // Okuma olayı (paralel olarak işlenecek)
        if (handleRead(clientSock) < 0) {
          perror("handleRead_");
          closeSocket(fd);
          io_uring_cqe_seen(&ring_, cqe);
          continue;
        }
        // Okuma işlemi başarılı ise yazma işlemi için event ekle
        clientSock->setWriter();
        io_uring_cqe_seen(
            &ring_, cqe);  // Okuma işlemi tamamlandı, okuma işlemini kaldır
        continue;
      }
      if (cqe->flags & IORING_CQE_F_MISS) {
        // Yazma olayı (paralel olarak işlenecek)
        if (handleWrite(clientSock) <= 0) {
          perror("handleWrite");
          closeSocket(fd);
        }
        // Yazma işlemi tamamlandı, yazma işlemini kaldır
        clientSock->removeWriter();
        io_uring_cqe_seen(&ring_, cqe);
        continue;
      }
      // Diğer olaylar
      std::cerr << "Diğer olaylar: " << cqe->flags << std::endl;
      closeSocket(fd);
      perror("Socket anlaşılmadık bir şekilde kapandı: ");
      io_uring_cqe_seen(&ring_, cqe);
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
  io_uring ring_;
  io_uring_cqe** events_;

  sockaddr_in addr_;
  int addrLen_;
};

#endif
