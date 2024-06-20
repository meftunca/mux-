#pragma once
#include <sstream>
#include <string>
#include "Header.hpp"
#include "ada/ada.h"
namespace muxpp {
struct ParsedRequest {
  int clientFd;
  std::string path;
  std::string protocol;
  std::string host;
  std::string port;
  std::string query;
  std::string body;
};
class Request {
 public:
  inline explicit Request(int clientFd);
  inline explicit Request(const ParsedRequest& parsedRequest);
  inline Request(const Request& parsedRequest);

  inline ~Request();
  // Auto delete constructor
  // Request(const Request &) = delete;
  Request& operator=(const Request&) = delete;

  inline void setMethod(const std::string& method);

  inline std::string getMethod();
  inline void setPath(const std::string_view& path);
  inline std::string_view getPath();
  inline void setProtocol(const std::string& protocol);
  inline std::string getProtocol();
  inline void setHost(const std::string& host);
  inline std::string getHost();
  inline void setPort(const std::string& port);
  inline std::string getPort();
  inline void setQuery(const std::string& query);
  inline std::string getQuery();
  inline void setBody(const std::string& body);
  inline std::string getBody();
  inline void setHeader(const std::string& key, const std::string& value);
  inline std::string getHeader(const std::string& key);
  inline void removeHeader(const std::string& key);
  inline void clearHeader();
  inline void setHashFragment(const std::string& hashFragment);
  inline std::string getHashFragment();

  std::unordered_map<std::string, std::string> getParams();
  std::unordered_map<std::string, std::string> getCookie();

  void setParams(const std::unordered_map<std::string, std::string>& params);
  void setCookie(const std::unordered_map<std::string, std::string>& cookie);

  inline std::string toString();

  inline void parse(const std::string& request);

 private:
  int clientFd_;
  std::string method_;
  std::string_view path_;
  std::string protocol_;
  std::string host_;
  std::string port_;
  std::unordered_map<std::string, std::string> params_;
  std::unordered_map<std::string, std::string> cookie_;
  std::string query_;
  std::string body_;
  std::string hashFragment_;
  muxpp::HTTPHeader headers_;
};

Request::Request(int clientFdx) : clientFd_(clientFdx) {}

Request::Request(const ParsedRequest& parsedRequest) {
  this->clientFd_ = parsedRequest.clientFd;
  this->path_ = parsedRequest.path;
  this->protocol_ = parsedRequest.protocol;
  this->host_ = parsedRequest.host;
  this->port_ = parsedRequest.port;
  this->query_ = parsedRequest.query;
  this->body_ = parsedRequest.body;
}

Request::Request(const Request& parsedRequest) {
  this->clientFd_ = parsedRequest.clientFd_;
  this->method_ = parsedRequest.method_;
  this->path_ = parsedRequest.path_;
  this->protocol_ = parsedRequest.protocol_;
  this->host_ = parsedRequest.host_;
  this->port_ = parsedRequest.port_;
  this->query_ = parsedRequest.query_;
  this->body_ = parsedRequest.body_;
  this->hashFragment_ = parsedRequest.hashFragment_;
  this->headers_ = parsedRequest.headers_;
}

Request::~Request() = default;

void Request::setMethod(const std::string& method) {
  this->method_ = method;
}

std::string Request::getMethod() {
  return this->method_;
}

void Request::setPath(const std::string_view& path) {
  this->path_ = path;
}

std::string_view Request::getPath() {
  return this->path_;
}

void Request::setProtocol(const std::string& protocol) {
  this->protocol_ = protocol;
}

std::string Request::getProtocol() {
  return this->protocol_;
}

void Request::setHost(const std::string& host) {
  this->host_ = host;
}

std::string Request::getHost() {
  return this->host_;
}

void Request::setPort(const std::string& port) {
  this->port_ = port;
}

std::string Request::getPort() {
  return this->port_;
}

void Request::setQuery(const std::string& query) {
  this->query_ = query;
}

std::string Request::getQuery() {
  return this->query_;
}

void Request::setBody(const std::string& body) {
  this->body_ = body;
}

std::string Request::getBody() {
  return this->body_;
}

void Request::setHeader(const std::string& key, const std::string& value) {
  headers_.setHeader(key, value);
}

std::string Request::getHeader(const std::string& key) {
  return headers_.getHeader(key);
}

void Request::removeHeader(const std::string& key) {
  headers_.removeHeader(key);
}

void Request::clearHeader() {
  headers_.clearHeader();
}

void Request::setHashFragment(const std::string& hashFragment) {
  this->hashFragment_ = hashFragment;
}

std::string Request::getHashFragment() {
  return this->hashFragment_;
}

std::string Request::toString() {
  std::string str;
  str += getMethod() + " " + getPath().data() + " " + getProtocol() + "\n";
  str += "Host: " + this->host_ + "\n";
  str += "Port: " + this->port_ + "\n";
  str += "Query: " + this->query_ + "\n";
  str += "Body: " + this->body_ + "\n";
  str += headers_.toString();
  return str;
}

void Request::parse(const std::string& rawRequest) {
  std::istringstream requestStream(rawRequest);
  std::string line;

  // İstek satırı (method, path, protocol)
  if (!std::getline(requestStream, line)) {
    throw std::runtime_error("Invalid request: Missing request line");
  }
  std::istringstream firstLineStream(line);
  std::string method;
  std::string path;
  std::string protocol;
  if (!(firstLineStream >> method >> path >> protocol)) {
    throw std::runtime_error("Invalid request: Malformed request line");
  }

  // URL ayrıştırma (ada-url ile)
  auto parseResult = ada::parse<ada::url>(this->path_);
  if (!parseResult) {
    throw std::runtime_error("Invalid request: Malformed URL");
  }
  ada::url parsedUrl = parseResult.value();
  // this->path = parsedUrl.pathname;
  // this->query = parsedUrl.search;
  setPath(parsedUrl.get_pathname());
  setQuery(parsedUrl.get_search());
  setHashFragment(parsedUrl.get_hash());

  // Host ve Port bilgisini Header'lardan al
  std::string hostHeader = this->headers_.getHeader("Host");
  if (hostHeader.empty()) {
    throw std::runtime_error("Invalid request: Missing Host header");
  }
  size_t colonPos = hostHeader.find(':');
  if (colonPos != std::string::npos) {
    this->host_ = hostHeader.substr(0, colonPos);
    this->port_ = hostHeader.substr(colonPos + 1);
  } else {
    this->host_ = hostHeader;
    this->port_ =
        (this->protocol_ == "HTTP/1.1") ? "80" : "443";  // Varsayılan port
  }

  // Boş satırı atla
  if (!std::getline(requestStream, line) || !line.empty()) {
    throw std::runtime_error(
        "Invalid request: Missing blank line after headers");
  }

  // Header'ları oku
  while (std::getline(requestStream, line) && !line.empty()) {
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) {
      throw std::runtime_error("Invalid request: Malformed header line");
    }
    this->setHeader(line.substr(0, colonPos), line.substr(colonPos + 2));
  }

  // Body'yi oku (eğer varsa)
  if (requestStream) {
    setBody(requestStream.str());

    // auto body = std::string(std::istreambuf_iterator<char>(requestStream),
    // {}); setBody(std::string(std::istreambuf_iterator<char>(requestStream),
    // {}));
  }
}

}  // namespace muxpp