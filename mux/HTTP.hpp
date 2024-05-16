

#pragma once
#include <iostream>
#include <string>
#include <sys/_types/_timeval.h>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <poll.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#ifdef __APPLE__
#include <sys/event.h> // macOS için kqueue
#else
#include <sys/epoll.h> // Linux için epoll
#endif
#endif

std::string createHttpResponse(const std::string &content,
                               const std::string &contentType = "text/plain") {
  std::string response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: " + contentType + "\r\n";
  response += "Content-Length: " + std::to_string(content.length()) + "\r\n";
  response += "\r\n"; // Boş satır
  response += content;
  return response;
}
class Socket {
public:
  Socket(int port = 3732) : m_serverSocket(-1), m_port(port) {}
  ~Socket() {
    closeServerSocket();
    std::cout << "Server socket closed." << std::endl;
  } // Destructor

  int setup(int backlog = 5) {
    createSocket();
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (m_serverSocket < 0) {
      throw std::runtime_error("Error creating socket.");
    }

    if (serverAddr.sin_port == 0) {
      throw std::runtime_error("Error sin_port socket.");
    } else if (bind(m_serverSocket, (struct sockaddr *)&serverAddr,
                    sizeof(serverAddr)) < 0) {
      throw std::runtime_error("Error binding socket.");
    }

    if (listen(m_serverSocket, backlog) < 0) {
      throw std::runtime_error("Error listening on socket.");
    }
    std::cout << "Server listening on port " << m_port << std::endl;
    return 0;
  }

  void closeSocket(int socket) { closeServerSocket(); }
  int getServerSocket() const { return m_serverSocket; }

  int acceptConnection(int socket) {
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    return accept(socket, (struct sockaddr *)&clientAddr, &clientAddrLen);
  }

  // İstek işleme fonksiyonu (örnek)
  int receiveData(int socket, char *buffer, int size, int kqOrEpollfd = -1) {
#ifdef _WIN32
    // Windows için en hızlı yöntem genellikle recv veya WSARecv'dir.
    return recv(socket, buffer, size, 0);

#elif defined(__APPLE__)
    // macOS için kqueue ile asenkron okuma
    struct kevent events[1];
    int ret = kevent(kqOrEpollfd, NULL, 0, events, 1,
                     NULL); // kqOrEpollfd, kqueue tanımlayıcısı
    if (ret > 0 && events[0].filter == EVFILT_READ) {
      return read(socket, buffer, size);
    } else {
      return -1; // Hata veya veri yok
    }

#else // Linux (varsayılan olarak epoll)
    // Linux için epoll ile asenkron okuma
    epoll_event events[1];
    int ret = epoll_wait(kqOrEpollfd, events, 1,
                         0); // kqOrEpollfd, epoll tanımlayıcısı
    if (ret > 0 && events[0].events & EPOLLIN) {
      return read(socket, buffer, size);
    } else {
      return -1; // Hata veya veri yok
    }

#endif
  }

  void sendData(int socket, const char *buffer, int size) {
    send(socket, buffer, size, 0);
  }
  void handleConnection(int clientSocket, int kqOrEpollfd = -1) {

    char buffer[1024] = {0};
    int bytesRead =
        receiveData(clientSocket, buffer, sizeof(buffer) - 1, kqOrEpollfd);
    // std::cout << "Buffer:\t" << buffer << std::endl;
    if (bytesRead > 0) {
      buffer[bytesRead] = '\0'; // Null-terminate the string

      std::string response = createHttpResponse("Hello from server!");
      sendData(clientSocket, response.c_str(), response.size());
      closeClientSocket(clientSocket);
    }
  }

  void handleAndResumeConnections() {

    // Platform bağımlı kısım
#ifdef _WIN32
    std::vector<pollfd> fds;
    fds.push_back({m_serverSocket, POLLIN, 0});
#elif defined(__APPLE__)
    int kq = kqueue();
    struct kevent event;
    EV_SET(&event, m_serverSocket, EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(kq, &event, 1, NULL, 0, NULL);
#else
    int epollfd = epoll_create1(0);
    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = m_serverSocket;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, m_serverSocket, &event);
#endif

    int max_sd = m_serverSocket;

    while (true) {
      // Platform bağımlı bekleme
#ifdef _WIN32
      int activity = poll(fds.data(), fds.size(), -1);
#elif defined(__APPLE__)
      struct kevent events[10];
      int activity = kevent(kq, NULL, 0, events, 10, NULL);
#else
      epoll_event events[10];
      int activity = epoll_wait(epollfd, events, 10, -1);
#endif

      if (activity < 0) {
        // Hata yönetimi
        perror("poll/kqueue/epoll");
        break;
      }

      // Aktif soketleri kontrol et ve işle
      for (int i = 0; i < activity; i++) {
#ifdef _WIN32
        if (fds[i].revents & POLLIN) {
          if (fds[i].fd == m_serverSocket) {
            // Yeni bağlantı kabul etme
            int new_socket = acceptConnection(m_serverSocket);
            if (new_socket < 0) {
              std::cerr << "Error accepting connection." << std::endl;
              continue;
            }
            std::cout << "Client connected." << std::endl;
            fds.push_back({new_socket, POLLIN, 0});
            max_sd = std::max(max_sd, new_socket);
          } else {
            // İstemciden veri geldi
            handleConnection(fds[i].fd);
            closeClientSocket(fds[i].fd);
            fds.erase(fds.begin() + i); // Soket kapatıldı, listeden çıkar
            i--;                        // İndisi düzelt
          }
        }
#elif defined(__APPLE__)
        if (events[i].filter == EVFILT_READ) {
          if (events[i].ident == m_serverSocket) {
            // Yeni bağlantı kabul etme
            int new_socket = acceptConnection(m_serverSocket);
            struct kevent new_event;
            EV_SET(&new_event, new_socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
            kevent(kq, &new_event, 1, NULL, 0, NULL);
            max_sd = std::max(max_sd, new_socket);
          } else {
            // İstemciden veri geldi
            handleConnection(events[i].ident, kq);
            close(events[i].ident);
          }
        }
#else
        if (events[i].events & EPOLLIN) {
          if (events[i].data.fd == m_serverSocket) {
            // Yeni bağlantı kabul etme
            int new_socket = acceptConnection(m_serverSocket);
            if (new_socket < 0) {
              std::cerr << "Error accepting connection." << std::endl;
              continue;
            }
            std::cout << "Client connected." << std::endl;
            epoll_event new_event;
            new_event.events = EPOLLIN;
            new_event.data.fd = new_socket;
            epoll_ctl(epollfd, EPOLL_CTL_ADD, new_socket, &new_event);
            max_sd = std::max(max_sd, new_socket);
          } else {
            // İstemciden veri geldi
            handleConnection(events[i].data.fd, epollfd);
            close(events[i].data.fd);
          }
        }
#endif
      }
    }

#ifdef _WIN32
    closesocket(m_serverSocket);
    WSACleanup();
#elif defined(__APPLE__)
    close(kq);
#else
    close(epollfd);
#endif
    close(m_serverSocket);
  }

  void acceptNewConnections(fd_set &readfds, fd_set &master, int &max_sd) {
    int new_socket = acceptConnection(m_serverSocket);
    if (new_socket < 0) {
      std::cerr << "Error accepting connection." << std::endl;
      return;
    }
    std::cout << "Client connected." << std::endl;
    FD_SET(new_socket, &master);
    max_sd = std::max(max_sd, new_socket);
  }

  void handleExistingConnections(fd_set &readfds, fd_set &master, int &max_sd) {
    for (int i = 0; i <= max_sd; i++) {
      if (FD_ISSET(i, &readfds) && i != m_serverSocket) {
        handleConnection(i);
        closeClientSocket(i);
        FD_CLR(i, &master);
      }
    }
  }
  void closeClientSocket(int socket) {
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
  }
  void closeServerSocket() {
#ifdef _WIN32
    closesocket(m_serverSocket);
    WSACleanup();

#else
    close(m_serverSocket);
#endif
  }
  void createSocket() {

    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket < 0) {
      throw std::runtime_error("Error creating socket.");
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
  }

private:
  int m_serverSocket;
  int m_port;
  sockaddr_in serverAddr;
  std::mutex maxSdMutex;  // max_sd değişkeni için mutex
  std::mutex socketMutex; // Soket kapatma işlemleri için mutex
  struct timeval timeout = {
      .tv_sec = 1, .tv_usec = 0}; // Select fonksiyonunun timeout süresi
  // ...
};
/*
Example usage:
int main() {
  Socket server(3732); // Port numarasını isteğe göre değiştirin
  try {
    if (server.setup() < 0) {
      throw std::runtime_error("Error on setup");
    }
    server.handleConnections();
  } catch (const std::runtime_error &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
*/