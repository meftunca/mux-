// #include "Socket/SocketBase/Kqueue.hpp"
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <string>
#include "Socket/Poller/Server.hpp"
// #include "Socket/SocketBase/main.hpp"

int main() {
  CreateSocketT config;
  config.port = 3733;
  // config.default_buffer_size = 4096;

  SocketBase serverSocket(std::move(config));
  // multi-threading
  while (true) {
    serverSocket.handler(
        [&](auto client) -> int {
          char buffer[config.default_buffer_size];
          int bytesRead;
          bytesRead = client->read(buffer, config.default_buffer_size - 1);
          if (bytesRead > 0) {
            // userRequest.request += buffer;
          } else if (bytesRead == 0) {  // bağlantı client tarafından kapatıldı
            // Bağlantı kapatıldı
            return -1;
          } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              // EWOLDBLOCK: Okunacak veri yok, daha sonra tekrar dene
              perror("Veri okunamadı:");
            } else {
              perror("Reading attempt exceeded");
              return -1;
            }
          }
          std::string response =
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/plain\r\n"
              "Content-Length: 12\r\n"
              "\r\n"
              "Hello World!";
          if (client->write(response) < 0) {
            perror("send");
            return -1;
          }
          // sleep for 1 second
          return 1;
        },
        [&](auto client) -> int { return 1; });
  }
  std::cout << "Döngüden çıkıldığı için işlem bitti\n";

  return 0;
}