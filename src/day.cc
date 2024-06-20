#include <chrono>
#include <cstdint>
#include <ctime>
#include <format>
#include <iomanip>
#include <iostream>
#include <string>

namespace time_format {
inline constexpr const char* kIsO8601 = "%Y-%m-%dT%H:%M:%S";
inline constexpr const char* kRfC3339 = "%Y-%m-%dT%H:%M:%SZ";
inline constexpr const char* kRfC3339Nano = "%Y-%m-%dT%H:%M:%S.%N%Z";
inline constexpr const char* kShortDate = "%Y-%m-%d";
inline constexpr const char* kLongDate = "%B %d, %Y";
inline constexpr const char* kShortTime = "%H:%M:%S";
inline constexpr const char* kLongTime = "%H:%M:%S %Z";
inline constexpr const char* kShortDateTime = "%Y-%m-%d %H:%M:%S";
inline constexpr const char* kLongDateTime = "%B %d, %Y %H:%M:%S %Z";

// ... diğer formatlar (isteğe bağlı)
}  // namespace time_format

// kYY = Year, kMO = Month, kDD = Day, kHH = Hour, kMM = Minute, kSS = Second, kMS = Millisecond, kNS = Nanosecond
enum class TimeUnits { kYY, kMO, kDD, kHH, kMM, kSS, kMS, kNS };

class Day {
 public:
  // Oluşturucular
  Day() : timePoint_(std::chrono::system_clock::now()) {}
  explicit Day(const std::chrono::system_clock::time_point& tp)
      : timePoint_(tp) {}
  explicit Day(const std::string& dateString,
               const std::string& format = time_format::kIsO8601) {
    std::tm tm = {};
    std::istringstream ss(dateString);
    ss >> std::get_time(&tm, format.c_str());
    if (ss.fail()) {
      throw std::invalid_argument("Invalid date string or format");
    }
    timePoint_ = std::chrono::system_clock::from_time_t(std::mktime(&tm));
  }
  explicit Day(const std::tm& timeStruct)
      : timePoint_(std::chrono::system_clock::from_time_t(
            std::mktime(const_cast<std::tm*>(&timeStruct)))) {}

  // Getters
  [[nodiscard]] int year() const {
    return static_cast<int>(getYear(timePoint_));
  }
  [[nodiscard]] int month() const {
    return static_cast<int>(getMonth(timePoint_));
  }
  [[nodiscard]] int date() const {
    return static_cast<int>(getDay(timePoint_));
  }
  [[nodiscard]] int hours() const {
    return static_cast<int>(getHour(timePoint_));
  }
  [[nodiscard]] int minutes() const {
    return static_cast<int>(getMinute(timePoint_));
  }
  [[nodiscard]] int seconds() const {
    return static_cast<int>(getSecond(timePoint_));
  }
  [[nodiscard]] int day() const {
    return static_cast<int>(getDayOfWeek(timePoint_));
  }  // 0: Pazar, 1: Pazartesi, ...

  // Formatlama
  [[nodiscard]] std::string format(
      const std::string&& formatString = time_format::kIsO8601) const {
    auto time = std::chrono::system_clock::to_time_t(timePoint_);
    std::tm tm = *std::localtime(&time);
    char buffer[100];  // Formatlanmış dize için yeterli büyüklükte bir tampon
    strftime(buffer, sizeof(buffer), formatString.c_str(), &tm);
    return std::string(buffer);
  }

  // Manipülasyon
  [[nodiscard]] Day add(std::chrono::duration<int64_t> duration) const {
    return Day(timePoint_ + duration);
  }
  [[nodiscard]] Day subtract(std::chrono::duration<int64_t> duration) const {
    return Day(timePoint_ - duration);
  }
  [[nodiscard]] Day startOf(const TimeUnits& unit) const {
    auto timeT = std::chrono::system_clock::to_time_t(timePoint_);
    std::tm tm = *std::localtime(&timeT);
    switch (unit) {
      case TimeUnits::kYY:
        tm.tm_mon = 0;
      case TimeUnits::kMO:
        tm.tm_mday = 1;
      case TimeUnits::kDD:
        tm.tm_hour = 0;
      case TimeUnits::kHH:
        tm.tm_min = 0;
      case TimeUnits::kMM:
        tm.tm_sec = 0;
      case TimeUnits::kSS:
        tm.tm_sec = 0;
        break;
      default:
        break;
    }
    return Day(tm);
  }
  [[nodiscard]] Day endOf(const TimeUnits& unit) const {
    auto timeT = std::chrono::system_clock::to_time_t(timePoint_);
    std::tm tm = *std::localtime(&timeT);
    switch (unit) {
      case TimeUnits::kYY:
        tm.tm_mon = 11;
      case TimeUnits::kMO:
        tm.tm_mday = 31;
      case TimeUnits::kDD:
        tm.tm_hour = 23;
      case TimeUnits::kHH:
        tm.tm_min = 59;
      case TimeUnits::kMM:
        tm.tm_sec = 59;
      case TimeUnits::kSS:
        tm.tm_sec = 59;
        break;
      default:
        break;
    }
    return Day(tm);
  }
  [[nodiscard]] Day endOf(
      const std::string& unit) const;  // İsteğe bağlı olarak eklenebilir

  // Karşılaştırma
  [[nodiscard]] bool isBefore(const Day& other) const {
    return timePoint_ < other.timePoint_;
  }
  [[nodiscard]] bool isSame(const Day& other) const {
    return timePoint_ == other.timePoint_;
  }
  [[nodiscard]] bool isAfter(const Day& other) const {
    return timePoint_ > other.timePoint_;
  }

  // Diğer
  [[nodiscard]] std::string toString() const {
    return format();
  }  // Varsayılan format: ISO8601
  [[nodiscard]] std::int64_t unix() const {

    return std::chrono::duration_cast<std::chrono::seconds>(
               timePoint_.time_since_epoch())
        .count();
  }

 private:
  std::chrono::system_clock::time_point timePoint_;

  static std::vector<std::string> timezones() {
    // Zaman dilimlerini döndüren bir fonksiyon
    std::chrono::get_tzdb();
    return {};
  }
  // Yardımcı fonksiyonlar (chrono ile time_t arasında dönüşüm için)
  static std::uint_fast64_t getYear(
      const std::chrono::system_clock::time_point& tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&tt);

    return tm.tm_year + 1900;
  }
  // ... diğer yardımcı fonksiyonlar (getMonth, getDay, getHour, getMinute, getSecond, getDayOfWeek)
  static std::uint_fast64_t getMonth(
      const std::chrono::system_clock::time_point& tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&tt);
    return tm.tm_mon + 1;
  }
  static std::uint_fast64_t getDay(
      const std::chrono::system_clock::time_point& tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&tt);
    return tm.tm_mday;
  }
  static std::uint_fast64_t getHour(
      const std::chrono::system_clock::time_point& tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&tt);
    return tm.tm_hour;
  }
  static std::uint_fast64_t getMinute(
      const std::chrono::system_clock::time_point& tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&tt);
    return tm.tm_min;
  }
  static std::uint_fast64_t getSecond(
      const std::chrono::system_clock::time_point& tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&tt);
    return tm.tm_sec;
  }
  static std::uint_fast64_t getDayOfWeek(
      const std::chrono::system_clock::time_point& tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&tt);
    return tm.tm_wday;
  }
};

int main() {
  try {
    Day day;
    std::cout << day.format() << std::endl;  // 2021-10-10T12:00:00
    std::cout << day.format(time_format::kLongDateTime)
              << std::endl;                    // October 10, 2021 12:00:00
    std::cout << day.toString() << std::endl;  // 2021-10-10T12:00:00
    std::cout << day.unix() << std::endl;      // 1633862400

    Day day2("2021-10-10T12:00:00");
    std::cout << day2.format() << std::endl;  // 2021-10-10T12:00:00
    std::cout << day2.format(time_format::kLongDateTime)
              << std::endl;                     // October 10, 2021 12:00:00
    std::cout << day2.toString() << std::endl;  // 2021-10-10T12:00:00
    std::cout << day2.unix() << std::endl;      // 1633862400

    Day day3(day2.add(std::chrono::hours(1)));
    std::cout << day3.format() << std::endl;  // 2021-10-10T13:00:00
    std::cout << day3.format(time_format::kLongDateTime)
              << std::endl;                     // October 10, 2021 13:00:00
    std::cout << day3.toString() << std::endl;  // 2021-10-10T13:00:00
    std::cout << day3.unix() << std::endl;      // 1633866000

    Day day4(day3.subtract(std::chrono::hours(1)));
    std::cout << day4.format() << std::endl;  // 2021-10-10T12:00:00
    std::cout << day4.format(time_format::kLongDateTime)
              << std::endl;                     // October 10, 2021 12:00:00
    std::cout << day4.toString() << std::endl;  // 2021-10-10
    std::cout << day4.unix() << std::endl;      // 1633862400
  } catch (const std::invalid_argument& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}