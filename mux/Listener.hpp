#include <functional>
#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

// İstek işleme fonksiyonu tipi (kullanıcının sağlayacağı fonksiyon)
using RequestHandler = std::function<void(const std::string &)>;

// Middleware sınıfı
class HttpMiddleware {
public:
  HttpMiddleware(RequestHandler handler) : requestHandler(handler) {}

  void start(int port = 3732) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
      std::cerr << "Error creating socket." << std::endl;
      return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    std::cout << "Port:" << serverAddr.sin_port << std::endl;
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) <
        0) {
      std::cerr << "Error binding socket." << std::endl;
      return;
    }

    listen(serverSocket, 5);
    std::cout << "Listening on port " << port << "..." << std::endl;

    while (true) {
      sockaddr_in clientAddr;
      socklen_t clientAddrLen = sizeof(clientAddr);
      int clientSocket =
          accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
      if (clientSocket < 0) {
        std::cerr << "Error accepting connection." << std::endl;
        continue;
      }
      std::cout << "Connection accepted." << std::endl;

      // İstemciyi ayrı bir iş parçacığında işleme
      std::thread([this, clientSocket]() {
        char buffer[1024];
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
          std::string request(buffer, bytesRead);
          std::cout << "Request received: " << request << std::endl;
          requestHandler(
              request); // Kullanıcının isteği işleyen fonksiyonunu çağır
        }
        std::cout << "Connection closed." << std::endl;
#ifdef _WIN32
        closesocket(clientSocket);
#else
        close(clientSocket);
#endif
      }).detach();
    }

#ifdef _WIN32
    closesocket(serverSocket);
    WSACleanup();
#else
    close(serverSocket);
#endif
  }

private:
  RequestHandler requestHandler;
};

int main() {
#ifdef _WIN32
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    std::cerr << "WSAStartup failed." << std::endl;
    return 1;
  }
#endif

  // Kullanıcının istekleri işleyen fonksiyonu
  RequestHandler userHandler = [](const std::string &request) {
    std::cout << "Request received: " << request << std::endl;
    // ... (istek işleme mantığı burada)
  };

  HttpMiddleware middleware(userHandler);
  middleware.start();

  return 0;
}
