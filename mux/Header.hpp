#pragma once
#include <string>
#include <unordered_map>
// HTTP Header
namespace muxpp {
class HTTPHeader {
public:
  HTTPHeader();
  ~HTTPHeader();
  inline void setHeader(const std::string &key, const std::string &value);
  inline std::string getHeader(const std::string &key);
  inline void removeHeader(const std::string &key);
  inline void clearHeader();
  inline std::string toString();

private:
  std::unordered_map<std::string, std::string> headers;
};

HTTPHeader::HTTPHeader() {}

HTTPHeader::~HTTPHeader() {}

void HTTPHeader::setHeader(const std::string &key, const std::string &value) {
  headers[key] = value;
}

std::string HTTPHeader::getHeader(const std::string &key) {
  return headers[key];
}

void HTTPHeader::removeHeader(const std::string &key) { headers.erase(key); }

void HTTPHeader::clearHeader() { headers.clear(); }

std::string HTTPHeader::toString() {
  std::string result;
  for (auto &header : headers) {
    result += header.first + ": " + header.second + "\r\n";
  }
  return result;
}
} // namespace muxpp