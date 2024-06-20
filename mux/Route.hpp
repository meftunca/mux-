// Unix And Windows HTTP Router for C++
// Supported HTTP Methods: GET, POST, PUT, DELETE, OPTIONS, PATCH, HEAD
// Supported Functions:
// Use,Group,Get,Post,Put,Delete,Options,Patch,Head,WebSocket,Listen

#pragma once  // Include guard

#include <string>

#include "Request.hpp"
#include "Response.hpp"

namespace muxpp {
using RequestHandler = std::function<void(Request*, Response*)>;

using MiddleWareFunc = std::function<Request*(RequestHandler)>;
enum class HTTPMethod {
  kGet,
  kPOST,
  kPUT,
  kDELETE,
  kOPTIONS,
  kPATCH,
  kHEAD,
  kANY
};
struct Route {
  std::string path;
  std::string method;
  RequestHandler handler;
  Route* parent;
};

}  // namespace muxpp