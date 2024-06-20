#include <__bit/rotate.h>
#include <unistd.h>
#include <coroutine>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>
#include "Socket/ClientSocket.hpp"
#include "Socket/Generator.hpp"

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
using HandleOperation =
    std::function<int(std::shared_ptr<ClientSocket>)>;  // (client) -> int
using CustomHashedMap =
    std::unordered_map<int, std::shared_ptr<ClientSocket>, IntHashBitRotate>;

#ifdef __APPLE__
#include <sys/event.h>
using PollingEvents = struct kevent*;
using PollingIdentifier = int;

inline PollingIdentifier CreatePollingNotificationInterface() {
  return kqueue();
}

inline bool setPollerReadableWritable(int socket_, int poller_ni_) {
  std::cout << "Okuma ve yazma izinleri veriliyor " << socket_ << " "
            << poller_ni_ << std::endl;

  struct kevent events[2];  // İki olay için kevent dizisi

  // Okuma olayı
  EV_SET(&events[0], socket_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);

  // Yazma olayı
  EV_SET(&events[1], socket_, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, nullptr);

  // Her iki olayı da kqueue'ya ekle
  if (kevent(poller_ni_, events, 2, nullptr, 0, nullptr) < 0) {
    std::perror("Okuma/yazma izinleri verilemedi");
    return false;
  }

  return true;
}

inline bool setPollerReadable(int socket_, int poller_ni_) {
  std::cout << "Okuma izinleri veriliyor " << socket_ << " " << poller_ni_
            << std::endl;
  struct timespec timeout = {0, 1000000};  // 1 milisaniyelik zaman aşımı
  struct kevent event;
  EV_SET(&event, socket_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
  if (kevent(poller_ni_, &event, 1, &event, 1, &timeout) < 0) {
    std::perror("Readable process is not assigned");
    return false;
  }

  return true;
}

inline bool setPollerWritable(int socket_, int poller_ni_) {
  std::cout << "Writable" << std::endl;

  struct timespec timeout = {0, 1000000};  // 1 milisaniyelik zaman aşımı
  struct kevent event;
  EV_SET(&event, socket_, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, nullptr);
  if (kevent(poller_ni_, &event, 1, &event, 1, &timeout) < 0) {
    std::perror("Writeable process is not assigned");
    return false;
  }
  return true;
}

// For döngüsü içindeki işlemleri bir coroutine haline getir
inline Generator<int> reduceEvents(
    int poller_ni_, int socket_, int clientID, int filter,
    CustomHashedMap& clientFds_, std::mutex& clientFdMutex_,
    HandleOperation&& handleRead, HandleOperation&& handleWrite,
    const std::function<bool(int)>& closeSocket) {

  if (clientID == socket_) {
    std::cout << "Yeni istemci bağlantısı" << std::endl;
    auto clientSock = std::make_shared<ClientSocket>(poller_ni_, socket_);

    auto clientFD = clientSock->accept();
    std::cout << "ClientFD: " << clientFD << std::endl;
    if (clientFD == -1) {
      perror("accept");
      co_return;
    }
    // Yeni istemci bağlantısı
    clientFds_[clientFD] = clientSock;
    std::cout << "New Connection Accepted: " << clientFD << std::endl;
    // clientFds_.emplace(clientFD, clientSock);
    // clientFds_.insert({clientFD, clientSock});
    co_await std::suspend_always{};
    co_return;
  }

  const auto clientSock = clientFds_[clientID];

  if (filter == EVFILT_READ) {
    // Okuma olayı (paralel olarak işlenecek)
    auto readStatus = handleRead(clientSock);
    co_await std::suspend_always{};
    if (readStatus < 0) {
      perror("handleRead_");
      closeSocket(clientID);
      co_await std::suspend_always{};
      co_return;
    }
    // Okuma işlemi başarılı ise yazma işlemi için event ekle
    //   clientSock->setWriter();
    co_return;
    //
  }

  if (filter == EVFILT_WRITE) {
    // Yazma olayı (paralel olarak işlenecek)
    auto writeStatus = handleWrite(clientSock);
    co_await std::suspend_always{};
    if (writeStatus < 0) {
      perror("handleWrite");
      closeSocket(clientID);
      co_await std::suspend_always{};
    }
    // std::cout << "Yazma işlemi tamamlandı\n";
    // // closeSocket(fd);
    // perror("Client Socket kapatıldı");
    // Yazma işlemi tamamlandı, yazma işlemini kaldır
    // removeClientWriter(poller_ni_, fd);
    //   clientSock->removeWriter();  //  yazma işlemini kaldır

    // closeSocket(fd);
    co_return;
  }
}

/*
    - 0 = Okuma işlemi başarılı bir şekilde gerçekleşti
    - -1 = Okuma işlemi başarısız
    - -2 = Okuma işlemi tamamlandı, yazma işlemi için event ekle
 */
inline void pollingHandler(int poller_ni_, int socket_, PollingEvents& events_,
                           int backlog, CustomHashedMap& clientFds_,
                           std::mutex& clientFdMutex_,
                           HandleOperation&& handleRead,
                           HandleOperation&& handleWrite,
                           const std::function<bool(int)>& closeSocket) {

  int numEvents = kevent(poller_ni_, nullptr, 0, events_, backlog, nullptr);
  if (numEvents == -1) {
    perror("kevent(numEvents)");
    return;
  }
  for (int i = 0; i < numEvents; ++i) {
    const auto event = events_[i];
    const int fd = event.ident;
    const int filter = event.filter;
    // humanize event
    // switch (filter) {
    //   case EVFILT_READ:
    //     std::cout << "Okuma olayı" << std::endl;
    //     break;
    //   case EVFILT_WRITE:
    //     std::cout << "Yazma olayı" << (fd == socket_) << std::endl;
    //     break;
    //   case EVFILT_USER:
    //     std::cout << "EVFILT_USER" << std::endl;
    //     break;
    //   case EVFILT_AIO:
    //     std::cout << "EVFILT_AIO" << std::endl;
    //     break;
    //   case EVFILT_VNODE:
    //     std::cout << "EVFILT_VNODE" << std::endl;
    //     break;
    //   case EVFILT_PROC:
    //     std::cout << "EVFILT_PROC" << std::endl;
    //     break;
    //   case EVFILT_SIGNAL:
    //     std::cout << "EVFILT_SIGNAL" << std::endl;
    //     break;
    //   case EVFILT_TIMER:
    //     std::cout << "EVFILT_TIMER" << std::endl;
    //     break;
    //   case EVFILT_MACHPORT:
    //     std::cout << "EVFILT_MACHPORT" << std::endl;
    //     break;
    //   case EVFILT_FS:
    //     std::cout << "EVFILT_FS" << std::endl;
    //     break;
    // }
    // std::cout << "fd: " << fd << " filter: " << filter << std::endl;

    // handle close

    if (fd == socket_) {
      // std::lock_guard<std::mutex> lock(
      //     clientFdMutex_);  // Kritik bölgeyi kilitle
      auto clientSock = std::make_shared<ClientSocket>(poller_ni_, socket_);

      auto clientFD = clientSock->accept();
      // auto clientFD = clientSock->accept();

      if (clientFD == -1) {
        perror("accept");
        continue;
      }
      // Yeni istemci bağlantısı
      clientFds_[clientFD] = clientSock;
      std::cout << "New Connection Accepted: " << clientFD << std::endl;
      // clientFds_.emplace(clientFD, clientSock);
      // clientFds_.insert({clientFD, clientSock});
      continue;
    }
    if (fd == -1) {
      std::cerr << "fd == -1" << std::endl;
      continue;
    }

    const auto clientSock = clientFds_[fd];

    // process ?
    if (filter == EVFILT_READ) {
      // Okuma olayı (paralel olarak işlenecek)
      auto readStatus = handleRead(clientSock);
      if (readStatus < 0) {
        perror("handleRead_");
        closeSocket(fd);
      }
      // Okuma işlemi başarılı ise yazma işlemi için event ekle
      //   clientSock->setWriter();
      // closeSocket(fd);
      continue;
      //
    }
    if (filter == EVFILT_WRITE) {
      // Yazma olayı (paralel olarak işlenecek)
      auto writeStatus = handleWrite(clientSock);
      if (writeStatus < 0) {
        perror("handleWrite");
        closeSocket(fd);
      }
      continue;
    }
    // Diğer olaylar
    if (event.flags & EV_EOF || event.flags & EV_ERROR) {
      perror("EV_EOF KqueueSocket handler ");
      // std::cout << "Beklenmedik bir hata oluştu: " << event.flags << std::endl;
      if (fd != socket_ && !closeSocket(fd)) {
        perror("Socket Kapatılıyor, çünkü beklenmedik bir hata oluştu");
        continue;
      }
      closeSocket(-1);
      return;
    }
  }
}

#elif __linux__
// io_uring aktif mi ?

// Eğer tanımı yapılmımışsa ve linux çekirdeği 5.1den büyükse IO_URING_ENABLE tanımını yap
#if !defined(IO_URING_ENABLE) && defined(__linux__) && \
    LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
#define IO_URING_ENABLE 1
#endif

#ifdef IO_URING_ENABLE == 1
#include <liburing.h>

using PollingEvents = struct io_uring_cqe*;
using PollingIdentifier = struct io_uring*;

#define MAX_EVENTS 100000
#define QUEUE_DEPTH 320000

inline PollingIdentifier CreatePollingNotificationInterface() {
  struct io_uring* ring = new io_uring;
  if (io_uring_queue_init(QUEUE_DEPTH, ring, 0) < 0) {
    delete ring;
    throw std::runtime_error("io_uring_queue_init");
  }
  return ring;
}

inline bool setPollerReadable(int socket_, struct io_uring* poller_ni_) {
  struct io_uring_sqe* sqe = io_uring_get_sqe(poller_ni_);
  io_uring_prep_poll_add(sqe, socket_, POLLIN);
  sqe->user_data = socket_;
  return io_uring_submit(poller_ni_) >= 0;
}

inline bool setPollerWritable(int socket_, struct io_uring* poller_ni_) {
  struct io_uring_sqe* sqe = io_uring_get_sqe(poller_ni_);
  io_uring_prep_poll_add(sqe, socket_, POLLOUT);
  sqe->user_data = socket_;
  return io_uring_submit(poller_ni_) >= 0;
}

inline void pollingHandler(struct io_uring* poller_ni_, int socket_,
                           PollingEvents events_, int backlog,
                           CustomHashedMap& clientFds_,
                           std::mutex& clientFdMutex_,
                           HandleOperation&& handleRead,
                           HandleOperation&& handleWrite,
                           const std::function<bool(int)>& closeSocket) {

  int numEvents = 0;
  io_uring_for_each_cqe(poller_ni_, events_, numEvents++) {
    struct io_uring_cqe* cqe = events_;

    int fd = static_cast<int>(cqe->user_data);

    if (cqe->res < 0) {
      // Hata durumu
      closeSocket(fd);
      continue;
    }

    if (fd == socket_) {
      // Yeni istemci bağlantısı
      std::unique_lock<std::mutex> lock(clientFdMutex_);
      auto clientSock = std::make_shared<ClientSocket>(poller_ni_, socket_);
      auto clientFD = clientSock->accept();
      if (clientFD == -1) {
        perror("accept");
        continue;
      }
      clientFds_[clientFD] = clientSock;
      continue;
    }

    const auto clientSock = clientFds_[fd];

    if (cqe->res & POLLIN) {
      // Okuma olayı
      if (handleRead(clientSock) < 0) {
        perror("handleRead_");
        closeSocket(fd);
        continue;
      }
      clientSock->setWriter();
    }

    if (cqe->res & POLLOUT) {
      // Yazma olayı
      if (handleWrite(clientSock) <= 0) {
        perror("handleWrite");
        closeSocket(fd);
      } else {
        clientSock->removeWriter();
      }
    }
  }
}
#else  // EPOLL işlemleri
#include <sys/epoll.h>

using PollingEvents = struct epoll_event*;
using PollingIdentifier = int;

#define MAX_EVENTS 100000

inline PollingIdentifier CreatePollingNotificationInterface() {
  return epoll_create1(0);  // Epoll instance'ı oluştur
}

inline bool setPollerReadable(int socket_, int poller_ni_) {
  struct epoll_event event;
  event.events = EPOLLIN | EPOLLET;  // Edge-triggered okuma olayı
  event.data.fd = socket_;
  return epoll_ctl(poller_ni_, EPOLL_CTL_ADD, socket_, &event) != -1;
}

inline bool setPollerWritable(int socket_, int poller_ni_) {
  struct epoll_event event;
  event.events = EPOLLOUT | EPOLLET;  // Edge-triggered yazma olayı
  event.data.fd = socket_;
  return epoll_ctl(poller_ni_, EPOLL_CTL_ADD, socket_, &event) != -1;
}

inline void pollingHandler(int poller_ni_, int socket_, PollingEvents events_,
                           int backlog, CustomHashedMap& clientFds_,
                           std::mutex& clientFdMutex_,
                           HandleOperation&& handleRead,
                           HandleOperation&& handleWrite,
                           const std::function<bool(int)>& closeSocket) {
  int numEvents = epoll_wait(poller_ni_, events_, backlog, -1);
  if (numEvents == -1) {
    perror("epoll_wait");
    return;
  }

  for (int i = 0; i < numEvents; ++i) {
    const auto& event = events_[i];
    const int fd = event.data.fd;

    if (event.events & EPOLLERR || event.events & EPOLLHUP) {
      // Hata veya bağlantı kesilmesi durumu
      closeSocket(fd);
      continue;
    }

    if (fd == socket_) {
      // Yeni istemci bağlantısı
      std::unique_lock<std::mutex> lock(clientFdMutex_);
      auto clientSock = std::make_shared<ClientSocket>(poller_ni_, socket_);
      auto clientFD = clientSock->accept();
      if (clientFD == -1) {
        perror("accept");
        continue;
      }
      clientFds_[clientFD] = clientSock;
      continue;
    }

    const auto clientSock = clientFds_[fd];

    if (event.events & EPOLLIN) {
      // Okuma olayı
      if (handleRead(clientSock) < 0) {
        perror("handleRead_");
        closeSocket(fd);
        continue;
      }
      clientSock->setWriter();
    }

    if (event.events & EPOLLOUT) {
      // Yazma olayı
      if (handleWrite(clientSock) <= 0) {
        perror("handleWrite");
        closeSocket(fd);
      } else {
        clientSock->removeWriter();
      }
    }
  }
}
#endif
#endif