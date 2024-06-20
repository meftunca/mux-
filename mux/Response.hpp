#pragma once

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "HTTPStatus.hpp"
#include "Header.hpp"
#include "helpers.hpp"
namespace muxpp {

// HTTP isteği sınıfı
class Response {
 public:
  Response() {}
  Response(int statusCode, std::string body, muxpp::HTTPHeader headers)
      : statusCode_(statusCode), body_(body), headers_(headers) {}
  int getStatusCode() const { return statusCode_; }
  std::string getBody() const { return body_; }
  muxpp::HTTPHeader getHeaders() const { return headers_; }
  void addHeader(const std::string& name, const std::string& value) {
    headers_.setHeader(name, value);
  }

  void removeHeader(const std::string& name) { headers_.removeHeader(name); }

  void setContentType(const std::string& type) {
    addHeader("Content-Type", type);
  }

  void setStatus(int status) { statusCode_ = status; }

  void setCookie(const std::string& name, const std::string& value) {
    cookies_[name] = value;
  }

  void removeCookie(const std::string& name) { cookies_.erase(name); }

  std::string to_string() {
    std::ostringstream oss;
    oss << protocol_ << " " << statusCode_ << " ";
    auto statusCodeStr = muxpp::http_status::reasonPhrase(statusCode_);
    if (statusCodeStr.empty()) {
      statusCodeStr = "Unknown";
    }
    oss << statusCodeStr << "\r\n";
    oss << "\r\n";
    auto headStr = headers_.toString();
    oss << headStr << "\r\n";
    oss << "\r\n" << body_;  // Gövdeyi ekle
    // Cookie'leri ekle
    auto cookies = mapToHeaderStr(this->cookies_, "=", "; ");
    oss << "Set-Cookie: " << cookies << "\r\n";
    return oss.str();
  }

 private:
  int statusCode_;
  std::string protocol_ = "HTTP/1.1";
  std::string body_;
  muxpp::HTTPHeader headers_;
  std::unordered_map<std::string, std::string> cookies_;
  // Durum kodlarının açıklamalarını içeren map'in tanımı
};

}  // namespace muxpp
