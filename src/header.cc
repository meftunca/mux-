// #include <iostream>
// #include <sstream>
// #include <string>
// #include <unordered_map>

// std::unordered_map<std::string, std::string>
// parseHttpHeaders(const std::string &headersStr) {
//   std::unordered_map<std::string, std::string> headers;
//   std::istringstream iss(headersStr);
//   std::string line;

//   while (std::getline(iss, line) && !line.empty()) {
//     size_t colonPos = line.find(':');
//     if (colonPos != std::string::npos) {
//       std::string headerName = line.substr(0, colonPos);
//       std::string headerValue =
//           line.substr(colonPos + 2); // +2, ':' ve boşluğu atlar
//       headers[headerName] = headerValue;
//     } else {
//       // Geçersiz başlık formatı
//       std::cerr << "Invalid header format: " << line << std::endl;
//     }
//   }

//   return headers;
// }

// int main() {
//   std::string headersStr = "Host: www.example.com\r\n"
//                            "User-Agent: Mozilla/5.0\r\n"
//                            "Accept: text/html\r\n"
//                            "\r\n"; // Boş satır

//   std::unordered_map<std::string, std::string> headers =
//       parseHttpHeaders(headersStr);

//   for (const auto &pair : headers) {
//     std::cout << pair.first << ": " << pair.second << std::endl;
//   }
//   return 0;
// }

#include "ada/ada.cpp"

#include <iostream>
#include <sstream>
#include <stdexcept> // std::runtime_error için
#include <string>
#include <unordered_map>
