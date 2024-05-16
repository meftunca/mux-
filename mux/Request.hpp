#pragma once
#include "Header.hpp"
#include "ada/ada.h"
#include <sstream>
#include <string>
namespace muxpp {
struct ParsedRequest {
  std::string path;
  std::string protocol;
  std::string host;
  std::string port;
  std::string query;
  std::string body;
};
class Request {
public:
  inline Request();
  inline Request(const ParsedRequest &parsedRequest);
  inline Request(const Request &parsedRequest);

  inline ~Request();
  // Auto delete constructor
  // Request(const Request &) = delete;
  Request &operator=(const Request &) = delete;

  inline void setMethod(const std::string &method);

  inline std::string getMethod();
  inline void setPath(const std::string_view &path);
  inline std::string_view getPath();
  inline void setProtocol(const std::string &protocol);
  inline std::string getProtocol();
  inline void setHost(const std::string &host);
  inline std::string getHost();
  inline void setPort(const std::string &port);
  inline std::string getPort();
  inline void setQuery(const std::string &query);
  inline std::string getQuery();
  inline void setBody(const std::string &body);
  inline std::string getBody();
  inline void setHeader(const std::string &key, const std::string &value);
  inline std::string getHeader(const std::string &key);
  inline void removeHeader(const std::string &key);
  inline void clearHeader();
  inline void setHashFragment(const std::string &hashFragment);
  inline std::string getHashFragment();
  inline std::string toString();

  inline void parse(const std::string &request);

private:
  std::string method;
  std::string_view path;
  std::string protocol;
  std::string host;
  std::string port;
  std::string query;
  std::string body;
  std::string hashFragment;
  muxpp::HTTPHeader headers;
};

Request::Request() {}

Request::Request(const ParsedRequest &parsedRequest) {
  this->path = parsedRequest.path;
  this->protocol = parsedRequest.protocol;
  this->host = parsedRequest.host;
  this->port = parsedRequest.port;
  this->query = parsedRequest.query;
  this->body = parsedRequest.body;
}

Request::Request(const Request &parsedRequest) {
  this->method = parsedRequest.method;
  this->path = parsedRequest.path;
  this->protocol = parsedRequest.protocol;
  this->host = parsedRequest.host;
  this->port = parsedRequest.port;
  this->query = parsedRequest.query;
  this->body = parsedRequest.body;
  this->hashFragment = parsedRequest.hashFragment;
  this->headers = parsedRequest.headers;
}

Request::~Request() {}

void Request::setMethod(const std::string &method) { this->method = method; }

std::string Request::getMethod() { return this->method; }

void Request::setPath(const std::string_view &path) { this->path = path; }

std::string_view Request::getPath() { return this->path; }

void Request::setProtocol(const std::string &protocol) {
  this->protocol = protocol;
}

std::string Request::getProtocol() { return this->protocol; }

void Request::setHost(const std::string &host) { this->host = host; }

std::string Request::getHost() { return this->host; }

void Request::setPort(const std::string &port) { this->port = port; }

std::string Request::getPort() { return this->port; }

void Request::setQuery(const std::string &query) { this->query = query; }

std::string Request::getQuery() { return this->query; }

void Request::setBody(const std::string &body) { this->body = body; }

std::string Request::getBody() { return this->body; }

void Request::setHeader(const std::string &key, const std::string &value) {
  headers.setHeader(key, value);
}

std::string Request::getHeader(const std::string &key) {
  return headers.getHeader(key);
}

void Request::removeHeader(const std::string &key) {
  headers.removeHeader(key);
}

void Request::clearHeader() { headers.clearHeader(); }

void Request::setHashFragment(const std::string &hashFragment) {
  this->hashFragment = hashFragment;
}

std::string Request::getHashFragment() { return this->hashFragment; }

std::string Request::toString() {
  std::string str = "";
  str += getMethod() + " " + getPath().data() + " " + getProtocol() + "\n";
  str += "Host: " + this->host + "\n";
  str += "Port: " + this->port + "\n";
  str += "Query: " + this->query + "\n";
  str += "Body: " + this->body + "\n";
  str += headers.toString();
  return str;
}

void Request::parse(const std::string &rawRequest) {
  std::istringstream requestStream(rawRequest);
  std::string line;

  // İstek satırı (method, path, protocol)
  if (!std::getline(requestStream, line)) {
    throw std::runtime_error("Invalid request: Missing request line");
  }
  std::istringstream firstLineStream(line);
  std::string method, path, protocol;
  if (!(firstLineStream >> method >> path >> protocol)) {
    throw std::runtime_error("Invalid request: Malformed request line");
  }

  // URL ayrıştırma (ada-url ile)
  auto parseResult = ada::parse<ada::url>(this->path);
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
  std::string hostHeader = this->headers.getHeader("Host");
  if (hostHeader.empty()) {
    throw std::runtime_error("Invalid request: Missing Host header");
  }
  size_t colonPos = hostHeader.find(':');
  if (colonPos != std::string::npos) {
    this->host = hostHeader.substr(0, colonPos);
    this->port = hostHeader.substr(colonPos + 1);
  } else {
    this->host = hostHeader;
    this->port =
        (this->protocol == "HTTP/1.1") ? "80" : "443"; // Varsayılan port
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

} // namespace muxpp