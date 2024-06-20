#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>

std::string_view Response200 =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: ";

int main() {
  int serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd == -1) {
    perror("Socket oluşturma hatası");
    return 1;
  }

  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(3732);
  serverAddr.sin_addr.s_addr = INADDR_ANY;

  int optval = 1;
  setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

  if (bind(serverFd, reinterpret_cast<struct sockaddr*>(&serverAddr),
           sizeof(serverAddr)) == -1) {
    perror("Bind hatası");
    close(serverFd);
    return 1;
  }

  if (listen(serverFd, 5) == -1) {
    perror("Listen hatası");
    close(serverFd);
    return 1;
  }

  int kq = kqueue();
  if (kq == -1) {
    perror("Kqueue oluşturma hatası");
    close(serverFd);
    return 1;
  }

  struct kevent events[10];  // Maksimum 10 olayı aynı anda izleyebiliriz (bu
                             // sayı arttırılabilir)

  struct kevent change;
  EV_SET(&change, serverFd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
  if (kevent(kq, &change, 1, nullptr, 0, nullptr) == -1) {
    perror("Kqueue event kaydetme hatası");
    close(kq);
    close(serverFd);
    return 1;
  }

  std::vector<int> clientFds;

  while (true) {
    int numEvents = kevent(kq, nullptr, 0, events, 10,
                           nullptr);  // Bloklanarak olayları bekle
    std::cout << "Olay sayısı: " << numEvents << std::endl;
    for (int i = 0; i < numEvents; ++i) {
      int fd = events[i].ident;

      if (fd == serverFd) {
        // Yeni istemci bağlantısı
        int clientFd = accept(serverFd, nullptr, nullptr);
        if (clientFd == -1) {
          perror("Accept hatası");
          continue;
        }
        struct kevent clientChanges[2];
        EV_SET(&clientChanges[0], clientFd, EVFILT_READ, EV_ADD | EV_ENABLE, 0,
               0, nullptr);
        EV_SET(&clientChanges[1], clientFd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0,
               0, nullptr);
        if (kevent(kq, &change, 2, nullptr, 0, nullptr) == -1) {
          perror("Kqueue event kaydetme hatası");
          close(clientFd);
          continue;
        }
        clientFds.push_back(clientFd);
        std::cout << "Yeni istemci bağlandı: " << clientFd << std::endl;
      } else {
        // Mevcut istemciden veri oku ve geri gönder (echo)
        char buffer[1024];
        int bytesRead = recv(fd, buffer, sizeof(buffer), 0);
        std::cout << "Okunan veri: " << buffer << std::endl;
        if (bytesRead > 0) {

          send(fd, Response200.data(), Response200.size(), 0);
        } else if (bytesRead == 0) {
          // Bağlantı kapatıldı
          send(fd, Response200.data(), Response200.size(), 0);
          close(fd);
          clientFds.erase(std::remove(clientFds.begin(), clientFds.end(), fd),
                          clientFds.end());
          std::cout << "İstemci bağlantısı kapatıldı: " << fd << std::endl;
        } else {
          perror("Okuma hatası");
          close(fd);
          clientFds.erase(std::remove(clientFds.begin(), clientFds.end(), fd),
                          clientFds.end());
        }
      }
    }
  }

  close(kq);
  close(serverFd);
  return 0;
}
