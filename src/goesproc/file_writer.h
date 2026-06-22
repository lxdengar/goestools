#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>

#include "lib/timer.h"
#include "lrit/file.h"
#include "log.h"

// FileWriter writes files to disk.
// This is where overwrite logic and logging is handled.
class FileWriter {
public:
  FileWriter(const std::string& prefix, const std::shared_ptr<Logger>& logger);
  ~FileWriter();

  void setForce(bool force) {
    force_ = force;
  }

  void write(
    const std::string& path,
    const cv::Mat& mat,
    const Timer* t = nullptr,
    const nlohmann::json& fields = nlohmann::json::object());

  void write(
    const std::string& path,
    const std::vector<char>& data,
    const Timer* t = nullptr,
    const nlohmann::json& fields = nlohmann::json::object());

  void write(
    const std::string& path,
    const nlohmann::json& json,
    const Timer* t = nullptr,
    const nlohmann::json& fields = nlohmann::json::object());

  void writeHeader(
    const lrit::File& file,
    const std::string& path,
    const nlohmann::json& fields = nlohmann::json::object());

  const std::shared_ptr<Logger>& logger() const {
    return logger_;
  }

protected:
  bool tryWrite(const std::string& path);

  std::string buildPath(const std::string& path);

  nlohmann::json outputFields(
    const std::string& path,
    const Timer* t,
    const nlohmann::json& fields) const;

  void logWriteResult(
    bool ok,
    const std::string& path,
    const Timer* t,
    const nlohmann::json& fields);

  const std::string prefix_;
  std::shared_ptr<Logger> logger_;
  bool force_;
};
