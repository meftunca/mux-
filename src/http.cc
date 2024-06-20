#include "mux/HTTP.hpp"
#include "mux/App.hpp"
#include "mux/Router.hpp"

// #include <gperftools/profiler.h>
#include <iostream>
#include <string>
#include <vector>
int main() {
  auto App = muxpp::App("3736", "localhost");
  auto router = muxpp::Router(&App);
  router
      .Get("/",
           [](auto* req, auto* res) {
             // auto res = muxpp::Response(200, "Hello, World!",
             // muxpp::HTTPHeader()); req->respond(res);
           })
      .Get("/about",
           [](auto* req, auto* res) {
             // auto res = muxpp::Response(200, "About Us",
             // muxpp::HTTPHeader()); req->respond(res);
           })
      .Get("/contact", [](auto* req, auto* res) {
        // auto res = muxpp::Response(200, "Contact Us", muxpp::HTTPHeader());
        // req->respond(res);
      });
  router.Group("/api/v1", nullptr)
      .Get("/about",
           [](auto* req, auto* res) {
             // auto res = muxpp::Response(200, "About Us",
             // muxpp::HTTPHeader()); req->respond(res);
           })
      .Get("/contact", [](auto* req, auto* res) {
        // auto res = muxpp::Response(200, "Contact Us",
        // muxpp::HTTPHeader()); req->respond(res);
      });
  std::cout << App.printAllRoutes() << std::endl;
  App.Listen();
  Socket server(3736);  // Port numarasını isteğe göre değiştirin
  try {
    if (server.setup() < 0) {
      throw std::runtime_error("Error on setup");
    }
    server.handleAndResumeConnections();
  } catch (const std::runtime_error& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}