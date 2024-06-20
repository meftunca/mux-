// Unix And Windows HTTP Router for C++
// Supported HTTP Methods: GET, POST, PUT, DELETE, OPTIONS, PATCH, HEAD
// Supported Functions:
// Use,Group,Get,Post,Put,Delete,Options,Patch,Head,WebSocket,Listen

#pragma once  // Include guard

#include <string>
#include <unordered_map>

#include "Request.hpp"
#include "Route.hpp"
// #include "Route.hpp"
#include <iostream>
#include <vector>

namespace muxpp {

class App {
 public:
  explicit App(
      const std::string& port = "3732", const std::string& host = "localhost",
      const std::string& protocol = "HTTP/1.1",
      std::unordered_map<std::string, std::string>* certificates = nullptr) {
    this->port_ = port;
    this->host_ = host;
    this->protocol_ = protocol;
    if (certificates != nullptr) {
      this->certificates_ = *certificates;
    }
  };
  ~App() { std::cout << "App destroyed" << std::endl; };
  // Middleware
  App Use(MiddleWareFunc&& handler) && {

    middlewares_.push_back(handler);
    return std::move(*this);
  };

  void Listen() {
    std::cout << "Listening on " << host_ << ":" << port_ << std::endl;
  };
  std::string printAllRoutes() {
    std::string result;
    for (const auto& route : routes_) {
      result += route.method + " " + route.path + "\n";
    }
    return result;
  }
  void addHandler(Route& route) { routes_.push_back(route); }

 private:
  std::vector<Route> routes_;
  std::string host_;
  std::string port_;
  std::string protocol_;  // HTTP/1.1
  std::string basePath_;
  std::vector<MiddleWareFunc> middlewares_;
  std::unordered_map<std::string, std::string> certificates_;

  void createNewRoute(const std::string& path, const std::string& method,
                      const RequestHandler& handler) {
    Route route;
    route.path = path;
    route.method = method;
    route.handler = handler;
    route.parent = nullptr;
    routes_.push_back(route);
  }
};

}  // namespace muxpp