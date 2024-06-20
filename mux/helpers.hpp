#pragma once

// Map to header str
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

inline std::string
mapToHeaderStr(const std::unordered_map<std::string, std::string> &headers,
               char *colon = ": ", char *splitter = "\r\n") {
  std::ostringstream oss;
  for (const auto &pair : headers) {
    oss << pair.first << colon << pair.second << splitter;
  }
  oss << "\r\n";
  return oss.str();
}

// Example for cookie
/*
std::unordered_map<std::string, std::string> headers;
headers["Set-Cookie"] = "session=abc
headers["Set-Cookie"] = "user=def";
std::cout << mapToHeaderStr(headers);

*/