#include "managed_process.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>

ManagedProcess::ManagedProcess(QObject* parent)
  : QObject(parent) {
  logDirectory_ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (logDirectory_.isEmpty()) {
    logDirectory_ = QDir::homePath() + "/.local/state/goestools-station";
  }
  logDirectory_ += "/logs";

  connect(&process_, &QProcess::started, this, [this] {
    startedAt_ = QDateTime::currentDateTimeUtc();
    openLogFiles();
    setStatus(Status::Running);
    emit started(pid());
  });

  connect(&process_, &QProcess::readyReadStandardOutput, this, [this] {
    const auto text = QString::fromLocal8Bit(process_.readAllStandardOutput());
    writeLogChunk("stdout", text);
    emit stdoutText(text);
    emit outputCaptured(processName_, "stdout", text);
  });

  connect(&process_, &QProcess::readyReadStandardError, this, [this] {
    const auto text = QString::fromLocal8Bit(process_.readAllStandardError());
    writeLogChunk("stderr", text);
    emit stderrText(text);
    emit outputCaptured(processName_, "stderr", text);
  });

  connect(&process_,
          &QProcess::errorOccurred,
          this,
          [this](QProcess::ProcessError) {
            setStatus(Status::Failed);
            emit failed(process_.errorString());
          });

  connect(&process_,
          static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
            &QProcess::finished),
          this,
          [this](int code, QProcess::ExitStatus status) {
            exitCode_ = code;
            exitStatus_ = status;
            closeLogFiles();
            setStatus(Status::Finished);
            emit stopped(code, status);
          });
}

void ManagedProcess::configure(
    const QString& program,
    const QStringList& arguments,
    const QString& workingDirectory) {
  program_ = program.trimmed();
  arguments_ = arguments;
  workingDirectory_ = workingDirectory.trimmed();

  const auto baseName = QFileInfo(program_).fileName();
  processName_ = sanitizeName(baseName.isEmpty() ? "process" : baseName);
}

bool ManagedProcess::start() {
  if (program_.isEmpty()) {
    setStatus(Status::Failed);
    emit failed("No command configured.");
    return false;
  }

  if (process_.state() != QProcess::NotRunning) {
    return false;
  }

  exitCode_ = 0;
  exitStatus_ = QProcess::NormalExit;
  setStatus(Status::Starting);

  if (!workingDirectory_.isEmpty()) {
    process_.setWorkingDirectory(workingDirectory_);
  } else {
    process_.setWorkingDirectory(QString());
  }

  process_.start(program_, arguments_);
  return true;
}

void ManagedProcess::stop() {
  if (process_.state() == QProcess::NotRunning) {
    return;
  }

  setStatus(Status::Stopping);
  process_.terminate();
  QTimer::singleShot(3000, this, [this] {
    if (process_.state() != QProcess::NotRunning) {
      process_.kill();
    }
  });
}

bool ManagedProcess::restart() {
  if (process_.state() != QProcess::NotRunning) {
    stop();
    connect(&process_,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
              &QProcess::finished),
            this,
            [this](int, QProcess::ExitStatus) {
              start();
            },
            Qt::SingleShotConnection);
    return true;
  }

  return start();
}

ManagedProcess::Status ManagedProcess::status() const {
  return status_;
}

QString ManagedProcess::statusText() const {
  switch (status_) {
  case Status::Idle:
    return "Idle";
  case Status::Starting:
    return "Starting";
  case Status::Running:
    return "Running";
  case Status::Stopping:
    return "Stopping";
  case Status::Finished:
    return "Finished";
  case Status::Failed:
    return "Failed";
  }

  return "Unknown";
}

qint64 ManagedProcess::pid() const {
  if (process_.state() == QProcess::NotRunning) {
    return 0;
  }
  return process_.processId();
}

qint64 ManagedProcess::uptimeMs() const {
  if (!startedAt_.isValid() || process_.state() == QProcess::NotRunning) {
    return 0;
  }
  return startedAt_.msecsTo(QDateTime::currentDateTimeUtc());
}

QString ManagedProcess::program() const {
  return program_;
}

QStringList ManagedProcess::arguments() const {
  return arguments_;
}

QString ManagedProcess::workingDirectory() const {
  return workingDirectory_;
}

QString ManagedProcess::processName() const {
  return processName_;
}

QString ManagedProcess::logDirectory() const {
  return logDirectory_;
}

int ManagedProcess::exitCode() const {
  return exitCode_;
}

QProcess::ExitStatus ManagedProcess::exitStatus() const {
  return exitStatus_;
}

QString ManagedProcess::errorString() const {
  return process_.errorString();
}

void ManagedProcess::setStatus(Status status) {
  if (status_ == status) {
    return;
  }

  status_ = status;
  emit statusChanged(status_);
}

void ManagedProcess::openLogFiles() {
  QDir().mkpath(logDirectory_);

  const auto prefix = logDirectory_ + "/" + processName_;
  stdoutLog_.setFileName(prefix + "-stdout.log");
  stderrLog_.setFileName(prefix + "-stderr.log");
  combinedLog_.setFileName(prefix + "-combined.log");

  const auto mode = QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text;
  if (!stdoutLog_.open(mode)) {
    emit failed("Unable to open stdout log: " + stdoutLog_.errorString());
  }
  if (!stderrLog_.open(mode)) {
    emit failed("Unable to open stderr log: " + stderrLog_.errorString());
  }
  if (!combinedLog_.open(mode)) {
    emit failed("Unable to open combined log: " + combinedLog_.errorString());
  }
}

void ManagedProcess::closeLogFiles() {
  stdoutLog_.close();
  stderrLog_.close();
  combinedLog_.close();
}

void ManagedProcess::writeLogChunk(const QString& stream, const QString& text) {
  QFile* streamLog = stream == "stderr" ? &stderrLog_ : &stdoutLog_;
  if (streamLog->isOpen()) {
    QTextStream out(streamLog);
    out << text;
    out.flush();
  }

  if (combinedLog_.isOpen()) {
    QTextStream out(&combinedLog_);
    out << "["
        << QDateTime::currentDateTimeUtc().toString(Qt::ISODate)
        << "] "
        << stream
        << "\n"
        << text;
    if (!text.endsWith('\n')) {
      out << "\n";
    }
    out.flush();
  }
}

QString ManagedProcess::sanitizeName(const QString& name) {
  QString out;
  out.reserve(name.size());
  for (const auto ch : name) {
    if (ch.isLetterOrNumber() || ch == '-' || ch == '_') {
      out.append(ch);
    } else {
      out.append('_');
    }
  }
  return out.isEmpty() ? "process" : out;
}
