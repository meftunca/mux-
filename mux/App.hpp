// Unix And Windows HTTP Router for C++
// Supported HTTP Methods: GET, POST, PUT, DELETE, OPTIONS, PATCH, HEAD
// Supported Functions:
// Use,Group,Get,Post,Put,Delete,Options,Patch,Head,WebSocket,Listen

#pragma once // Include guard

#include <string>
#include <unordered_map>

#include "Header.hpp"
#include "Route.hpp"
#include <iostream>
#include <thread>
#include <vector>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace muxpp {
struct Route {
  std::string path;
  std::string method;
  std::function<void()> handler;
  Route *parent;
};
class App {
public:
  App();
  ~App();
  void Use(const std::string &path, std::function<void()> handler);
  void Group(const std::string &path, std::function<void()> handler);
  void Get(const std::string &path, std::function<void()> handler);
  void Post(const std::string &path, std::function<void()> handler);
  void Put(const std::string &path, std::function<void()> handler);
  void Delete(const std::string &path, std::function<void()> handler);
  void Options(const std::string &path, std::function<void()> handler);
  void Patch(const std::string &path, std::function<void()> handler);
  void Head(const std::string &path, std::function<void()> handler);
  void Any(const std::string &path, std::function<void()> handler);
  void WebSocket(const std::string &path, std::function<void()> handler);
  void Listen(const std::string &port);
  void Listen(const std::string &port, const std::string &host);
  void Listen(const std::string &port, const std::string &host,
              const std::string &protocol);

private:
  std::vector<Route> routes;
  std::string host;
  std::string port;
  std::string protocol;
  int server_fd;
  int new_socket;
  int valread;
  struct sockaddr_in address;
  int addrlen;
  char buffer[1024] = {0};
  std::string response = "HTTP/1.1 200 OK\nContent-Type: "
                         "text/plain\nContent-Length: 12\n\nHello world!";
};

} // namespace muxpp