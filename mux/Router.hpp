// Unix And Windows HTTP Router for C++
// Supported HTTP Methods: GET, POST, PUT, DELETE, OPTIONS, PATCH, HEAD
// Supported Functions:
// Use,Group,Get,Post,Put,Delete,Options,Patch,Head,WebSocket,Listen

#pragma once  // Include guard

#include <string>
#include <unordered_map>

#include "App.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Route.hpp"
// #include "Route.hpp"
#include <iostream>
#include <utility>
#include <vector>

namespace muxpp {
class Router {
 public:
  explicit Router(const App* app, const std::string& basePath = "") {
    this->app_ = const_cast<App*>(app);
    this->basePath_ = basePath;
  };
  ~Router() { std::cout << "Router destroyed" << std::endl; };
  // Middleware
  Router Use(const MiddleWareFunc& handler) & {
    middlewares_.push_back(handler);
    return std::move(*this);
  };
  // Group example: /api/v1
  Router Group(const std::string& path, MiddleWareFunc* handler = nullptr) {
    Router subRouter = Router(app_, basePath_ + path);
    // subRouter.Use(handler);
    if (handler != nullptr) {
      subRouter.Use(*handler);
    }
    return subRouter;
  }
  // HTTP Methods
  Router Get(const std::string& path, const RequestHandler& handler) {
    createNewRoute(path, "GET", handler);
    return std::move(*this);
  };
  Router Post(const std::string& path, const RequestHandler& handler) {
    createNewRoute(path, "POST", handler);
    return std::move(*this);
  };
  Router Put(const std::string& path, const RequestHandler& handler) {
    createNewRoute(path, "PUT", handler);
    return std::move(*this);
  };
  Router Delete(const std::string& path, const RequestHandler& handler) {
    createNewRoute(path, "DELETE", handler);
    return std::move(*this);
  };
  Router Options(const std::string& path, const RequestHandler& handler) {
    createNewRoute(path, "OPTIONS", handler);
    return std::move(*this);
  };
  Router Patch(const std::string& path, const RequestHandler& handler) {
    createNewRoute(path, "PATCH", handler);
    return std::move(*this);
  };
  Router Head(const std::string& path, const RequestHandler& handler) {
    createNewRoute(path, "HEAD", handler);
    return std::move(*this);
  };
  Router Any(const std::string& path,
             const RequestHandler& handler);  // kaldırsak mı?

  Router WebSocket(const std::string& path, const RequestHandler& /*handler*/) {
    std::cout << "WebSocket is not implemented yet. : " << path << std::endl;
    return std::move(*this);
  };

  void createNewRoute(const std::string& path, const std::string& method,
                      const RequestHandler& handler) {
    if (app_ == nullptr) {
      throw std::runtime_error("App is not initialized.");
    }
    Route route;
    route.path = basePath_ + path;
    route.method = method;
    route.handler = handler;
    route.parent = nullptr;
    app_->addHandler(route);
  }

 private:
  App* app_;
  std::string basePath_;
  std::vector<MiddleWareFunc> middlewares_;
  std::unordered_map<std::string, std::string> certificates_;
};

}  // namespace muxpp