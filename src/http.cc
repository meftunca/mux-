#include "mux/HTTP.hpp"

#include <iostream>
#include <string>
#include <vector>

int main() {
  Socket server(3732); // Port numarasını isteğe göre değiştirin
  try {
    if (server.setup() < 0) {
      throw std::runtime_error("Error on setup");
    }
    server.handleAndResumeConnections();
  } catch (const std::runtime_error &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}