#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <set>
#include <string>

#include <nlohmann/json.hpp>

enum class LogLevel {
  QUIET = -1,
  ERROR = 0,
  WARNING = 1,
  INFO = 2,
  DEBUG = 3,
};

enum class LogFormat {
  TEXT,
  JSON,
};

const char* logLevelName(LogLevel level);
const char* logFormatName(LogFormat format);
bool parseLogLevel(const std::string& value, LogLevel* out);
bool parseLogFormat(const std::string& value, LogFormat* out);

class Logger {
public:
  Logger(LogLevel level, LogFormat format, unsigned summaryIntervalSeconds);

  bool enabled(LogLevel level) const;
  bool progressAllowed() const;

  void event(
    LogLevel level,
    const std::string& name,
    const nlohmann::json& fields = nlohmann::json::object());

  void eventRateLimited(
    LogLevel level,
    const std::string& name,
    const std::string& key,
    const nlohmann::json& fields,
    unsigned intervalSeconds = 10);

  void increment(const std::string& counter, uint64_t amount = 1);
  bool observeSpacecraft(const std::string& spacecraft);
  void tick();
  void summary(bool force = false);

private:
  struct RateLimitState {
    std::chrono::steady_clock::time_point last;
    uint64_t suppressed{0};
  };

  LogLevel level_;
  LogFormat format_;
  std::chrono::seconds summaryInterval_;
  std::chrono::steady_clock::time_point summaryStart_;
  std::map<std::string, uint64_t> counters_;
  std::set<std::string> intervalSpacecraft_;
  std::set<std::string> observedSpacecraft_;
  std::map<std::string, RateLimitState> rateLimits_;
};
