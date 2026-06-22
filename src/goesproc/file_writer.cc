#include "file_writer.h"

#include <sys/stat.h>

#include <fstream>

#include <util/fs.h>

#include "lrit/json.h"
#include "string.h"

using namespace util;

FileWriter::FileWriter(
    const std::string& prefix,
    const std::shared_ptr<Logger>& logger)
  : prefix_(prefix),
    logger_(logger) {
  force_ = false;
}

FileWriter::~FileWriter() {
}

nlohmann::json FileWriter::outputFields(
    const std::string& path,
    const Timer* t,
    const nlohmann::json& fields) const {
  auto output = fields;
  output["path"] = path;
  if (t) {
    output["duration_ms"] = static_cast<uint64_t>(t->elapsed().count() * 1000);
  }
  return output;
}

void FileWriter::logWriteResult(
    bool ok,
    const std::string& path,
    const Timer* t,
    const nlohmann::json& fields) {
  auto output = outputFields(path, t, fields);
  if (ok) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
      output["bytes"] = static_cast<uint64_t>(st.st_size);
    }
    logger_->increment("written");
    if (output.count("bytes") > 0) {
      logger_->increment("bytes_written", output["bytes"].get<uint64_t>());
    }
    logger_->event(LogLevel::INFO, "output_written", output);
  } else {
    logger_->increment("failed");
    logger_->event(LogLevel::ERROR, "output_failed", output);
  }
}

void FileWriter::write(
  const std::string& tail,
  const cv::Mat& mat,
  const Timer* t,
  const nlohmann::json& fields) {
  auto path = buildPath(tail);
  if (!tryWrite(path)) {
    logger_->increment("skipped");
    logger_->event(
      LogLevel::INFO,
      "output_skipped",
      outputFields(path, t, fields));
    return;
  }

  bool ok = false;
  try {
    ok = cv::imwrite(path, mat);
  } catch (const cv::Exception& error) {
    auto output = fields;
    output["error"] = error.what();
    logWriteResult(false, path, t, output);
    return;
  }
  auto output = fields;
  output["width"] = mat.cols;
  output["height"] = mat.rows;
  logWriteResult(ok, path, t, output);
}

void FileWriter::write(
  const std::string& tail,
  const std::vector<char>& data,
  const Timer* t,
  const nlohmann::json& fields) {
  auto path = buildPath(tail);
  if (!tryWrite(path)) {
    logger_->increment("skipped");
    logger_->event(
      LogLevel::INFO,
      "output_skipped",
      outputFields(path, t, fields));
    return;
  }

  std::ofstream of(path, std::ios::binary);
  of.write(data.data(), data.size());
  of.close();
  logWriteResult(static_cast<bool>(of), path, t, fields);
}

void FileWriter::write(
  const std::string& tail,
  const nlohmann::json& json,
  const Timer* t,
  const nlohmann::json& fields) {
  auto path = buildPath(tail);
  if (!tryWrite(path)) {
    logger_->increment("skipped");
    logger_->event(
      LogLevel::INFO,
      "output_skipped",
      outputFields(path, t, fields));
    return;
  }

  std::ofstream of(path);
  of << json;
  of.close();
  logWriteResult(static_cast<bool>(of), path, t, fields);
}

void FileWriter::writeHeader(
    const lrit::File& file,
    const std::string& path,
    const nlohmann::json& fields) {
  auto jsonHeader = lrit::toJSON(file);
  jsonHeader["Path"] = buildPath(path);
  auto jsonPath = removeSuffix(path) + ".json";
  write(jsonPath, jsonHeader, nullptr, fields);
}

bool FileWriter::tryWrite(const std::string& path) {
  struct stat st;

  auto rpos = path.rfind('/');
  if (rpos != std::string::npos) {
    mkdirp(path.substr(0, rpos));
  }

  auto rv = stat(path.c_str(), &st);
  if (rv < 0 && errno == ENOENT) {
    return true;
  }

  return force_;
}

std::string FileWriter::buildPath(const std::string& path) {
  if (prefix_ == ".") {
    return path;
  }
  return prefix_ + "/" + path;
}
