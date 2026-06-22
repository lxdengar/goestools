#include "log.h"

#include <cctype>
#include <iostream>

#include <util/time.h>

using nlohmann::json;

namespace {

std::string textValue(const json& value) {
  if (value.is_string()) {
    const auto text = value.get<std::string>();
    if (text.find_first_of(" \t\r\n\"") == std::string::npos) {
      return text;
    }
  }
  return value.dump();
}

} // namespace

const char* logLevelName(LogLevel level) {
  switch (level) {
  case LogLevel::QUIET:
    return "quiet";
  case LogLevel::ERROR:
    return "error";
  case LogLevel::WARNING:
    return "warning";
  case LogLevel::INFO:
    return "info";
  case LogLevel::DEBUG:
    return "debug";
  }
  return "unknown";
}

const char* logFormatName(LogFormat format) {
  return format == LogFormat::JSON ? "json" : "text";
}

bool parseLogLevel(const std::string& value, LogLevel* out) {
  if (value == "quiet") {
    *out = LogLevel::QUIET;
  } else if (value == "error") {
    *out = LogLevel::ERROR;
  } else if (value == "warning" || value == "warn") {
    *out = LogLevel::WARNING;
  } else if (value == "info") {
    *out = LogLevel::INFO;
  } else if (value == "debug") {
    *out = LogLevel::DEBUG;
  } else {
    return false;
  }
  return true;
}

bool parseLogFormat(const std::string& value, LogFormat* out) {
  if (value == "text") {
    *out = LogFormat::TEXT;
  } else if (value == "json") {
    *out = LogFormat::JSON;
  } else {
    return false;
  }
  return true;
}

Logger::Logger(
    LogLevel level,
    LogFormat format,
    unsigned summaryIntervalSeconds)
  : level_(level),
    format_(format),
    summaryInterval_(summaryIntervalSeconds),
    summaryStart_(std::chrono::steady_clock::now()) {
}

bool Logger::enabled(LogLevel level) const {
  if (level_ == LogLevel::QUIET) {
    return level == LogLevel::ERROR;
  }
  return static_cast<int>(level) <= static_cast<int>(level_);
}

bool Logger::progressAllowed() const {
  return format_ == LogFormat::TEXT && level_ != LogLevel::QUIET;
}

void Logger::event(
    LogLevel level,
    const std::string& name,
    const json& fields) {
  if (!enabled(level)) {
    return;
  }

  auto& stream =
    (level == LogLevel::ERROR || level == LogLevel::WARNING)
      ? std::cerr
      : std::cout;
  const auto timestamp = util::stringTime();

  if (format_ == LogFormat::JSON) {
    auto output = fields;
    output["timestamp"] = timestamp;
    output["level"] = logLevelName(level);
    output["event"] = name;
    stream << output.dump() << std::endl;
    return;
  }

  std::string levelName = level == LogLevel::WARNING
    ? "WARNING"
    : logLevelName(level);
  for (auto& c : levelName) {
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }

  stream << timestamp << " " << levelName << " " << name;
  for (auto it = fields.begin(); it != fields.end(); ++it) {
    stream << " " << it.key() << "=" << textValue(it.value());
  }
  stream << std::endl;
}

void Logger::eventRateLimited(
    LogLevel level,
    const std::string& name,
    const std::string& key,
    const json& fields,
    unsigned intervalSeconds) {
  const auto now = std::chrono::steady_clock::now();
  auto it = rateLimits_.find(key);
  if (it == rateLimits_.end()) {
    rateLimits_[key] = {now, 0};
    event(level, name, fields);
    return;
  }

  auto& state = it->second;
  if (now - state.last < std::chrono::seconds(intervalSeconds)) {
    state.suppressed++;
    return;
  }

  auto output = fields;
  if (state.suppressed > 0) {
    output["suppressed"] = state.suppressed;
  }
  state.last = now;
  state.suppressed = 0;
  event(level, name, output);
}

void Logger::increment(const std::string& counter, uint64_t amount) {
  counters_[counter] += amount;
}

bool Logger::observeSpacecraft(const std::string& spacecraft) {
  intervalSpacecraft_.insert(spacecraft);
  return observedSpacecraft_.insert(spacecraft).second;
}

void Logger::tick() {
  summary(false);
}

void Logger::summary(bool force) {
  if (summaryInterval_.count() == 0) {
    return;
  }

  const auto now = std::chrono::steady_clock::now();
  if (!force && now - summaryStart_ < summaryInterval_) {
    return;
  }

  if (counters_.empty() && intervalSpacecraft_.empty()) {
    summaryStart_ = now;
    return;
  }

  json fields = json::object();
  fields["interval_s"] = summaryInterval_.count();
  for (const auto& counter : counters_) {
    fields[counter.first] = counter.second;
  }
  if (!intervalSpacecraft_.empty()) {
    fields["spacecraft"] = json::array();
    for (const auto& spacecraft : intervalSpacecraft_) {
      fields["spacecraft"].push_back(spacecraft);
    }
  }

  event(LogLevel::INFO, "summary", fields);
  counters_.clear();
  intervalSpacecraft_.clear();
  summaryStart_ = now;
}
