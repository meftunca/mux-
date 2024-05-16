#pragma once

#include "HTTPStatus.hpp"
#include "Header.hpp"
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
namespace muxpp {

// HTTP isteği sınıfı
class Response {
public:
  Response() {}
  Response(int statusCode, std::string body, muxpp::HTTPHeader headers)
      : statusCode(statusCode), body(body), headers(headers) {}
  int getStatusCode() const { return statusCode; }
  std::string getBody() const { return body; }
  muxpp::HTTPHeader getHeaders() const { return headers; }
  void addHeader(const std::string &name, const std::string &value) {
    headers.setHeader(name, value);
  }

  void removeHeader(const std::string &name) { headers.removeHeader(name); }

  void setContentType(const std::string &type) {
    addHeader("Content-Type", type);
  }

  std::string to_string() {
    std::ostringstream oss;
    oss << protocol << " " << statusCode << " ";
    auto statusCodeStr = muxpp::HttpStatus::reasonPhrase(statusCode);
    if (statusCodeStr.empty()) {
      statusCodeStr = "Unknown";
    }
    oss << statusCodeStr << "\r\n";
    oss << "\r\n";
    auto headStr = headers.toString();
    oss << headStr << "\r\n";
    oss << "\r\n" << body; // Gövdeyi ekle

    return oss.str();
  }

private:
  int statusCode;
  std::string protocol = "HTTP/1.1";
  std::string body;
  muxpp::HTTPHeader headers;
  // Durum kodlarının açıklamalarını içeren map'in tanımı
};

} // namespace muxpp
