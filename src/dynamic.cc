// #include <functional>
// #include <iostream>
// #include <sstream>
// #include <string>
// #include <unordered_map>
// #include <vector>

// class DynamicRouter {
// public:
//   struct RouteMatch {
//     bool isMatch;
//     std::unordered_map<std::string, std::string> params;
//   };

//   // Dinamik rota ekleme
//   void addRoute(const std::string &pattern, std::function<void()> handler) {
//     std::vector<std::string> segments = splitPath(pattern);
//     routes.emplace_back(Route{segments, handler});
//   }

//   // Gelen yolu eşleştirme
//   RouteMatch matchRoute(const std::string &path) {
//     std::vector<std::string> pathSegments = splitPath(path);

//     for (const auto &route : routes) {
//       if (route.segments.size() != pathSegments.size()) {
//         continue; // Segment sayıları eşleşmiyorsa devam et
//       }

//       RouteMatch match{true, {}};
//       for (size_t i = 0; i < route.segments.size(); ++i) {
//         if (route.segments[i][0] == '{') {
//           // Dinamik segment ise, parametre değerini al
//           std::string paramName = route.segments[i].substr(
//               1, route.segments[i].size() - 2); // {} karakterlerini kaldır
//           match.params[paramName] = pathSegments[i];
//         } else if (route.segments[i] != pathSegments[i]) {
//           match.isMatch = false;
//           break; // Eşleşmeme durumu, sonraki rotaya geç
//         }
//       }

//       if (match.isMatch) {
//         return match; // Eşleşme bulundu
//       }
//     }

//     return {false, {}}; // Eşleşme bulunamadı
//   }

// private:
//   // Rota ve işleyici çifti
//   struct Route {
//     std::vector<std::string> segments;
//     std::function<void()> handler;
//   };

//   std::vector<Route> routes;

//   // Yardımcı fonksiyon: Yolu parçalara ayırma
//   std::vector<std::string> splitPath(const std::string &path) {
//     std::vector<std::string> segments;
//     std::istringstream iss(path);
//     std::string segment;
//     while (std::getline(iss, segment, '/')) {
//       if (!segment.empty()) {
//         segments.push_back(segment);
//       }
//     }
//     return segments;
//   }
// };

// int main() {
//   DynamicRouter router;
//   router.addRoute("/users/{id}/friends/{name}", []() {
//     std::cout << "User route matched!" << std::endl;
//   }); // handler yerine uygun bir fonksiyon pointer
//       // veya lambda ifadesi kullanabilirsiniz

//   DynamicRouter::RouteMatch match =
//       router.matchRoute("/users/123/friends/Alice");
//   if (match.isMatch) {
//     std::cout << "Route matched!" << std::endl;
//     for (const auto &param : match.params) {
//       std::cout << param.first << ": " << param.second << std::endl;
//     }
//   } else {
//     std::cout << "Route not found!" << std::endl;
//   }

//   return 0;
// }

#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>

bool match(const std::string &templateUrl, const std::string &url,
           std::unordered_map<std::string, std::string> &params) {
  std::regex re("\\{([^}]+)\\}");
  std::smatch match;

  std::string pattern = std::regex_replace(
      templateUrl, re,
      "([^/]+)"); // Dinamik kısımları yakalamak için regex deseni oluştur

  if (std::regex_match(url, match, std::regex(pattern))) {
    std::istringstream iss(templateUrl);
    std::string segment;
    int i = 1; // İlk eşleşme match[0] olduğu için 1'den başlıyoruz

    while (std::getline(iss, segment, '/')) {
      if (segment[0] == '{') {
        std::string paramName =
            segment.substr(1, segment.size() - 2); // {} karakterlerini kaldır
        params[paramName] = match[i];
        i++;
      }
    }
    return true; // Eşleşme bulundu
  }

  return false; // Eşleşme bulunamadı
}

int main() {
  std::string templateUrl = "/users/{id}";
  std::string url = "/users/123/ada";
  std::unordered_map<std::string, std::string> params;

  if (match(templateUrl, url, params)) {
    std::cout << "Route matched!" << std::endl;
    for (const auto &param : params) {
      std::cout << param.first << ": " << param.second << std::endl;
    }
  } else {
    std::cout << "Route not found!" << std::endl;
  }

  return 0;
}
