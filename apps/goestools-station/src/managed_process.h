#pragma once

#include <QDateTime>
#include <QFile>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

class ManagedProcess : public QObject {
  Q_OBJECT

public:
  enum class Status {
    Idle,
    Starting,
    Running,
    Stopping,
    Finished,
    Failed,
  };

  explicit ManagedProcess(QObject* parent = nullptr);

  void configure(
    const QString& program,
    const QStringList& arguments,
    const QString& workingDirectory = QString());

  bool start();
  void stop();
  bool restart();

  Status status() const;
  QString statusText() const;
  qint64 pid() const;
  qint64 uptimeMs() const;
  QString program() const;
  QStringList arguments() const;
  QString workingDirectory() const;
  QString processName() const;
  QString logDirectory() const;
  int exitCode() const;
  QProcess::ExitStatus exitStatus() const;
  QString errorString() const;

signals:
  void statusChanged(ManagedProcess::Status status);
  void started(qint64 pid);
  void stopped(int exitCode, QProcess::ExitStatus exitStatus);
  void failed(const QString& error);
  void stdoutText(const QString& text);
  void stderrText(const QString& text);
  void outputCaptured(
    const QString& processName,
    const QString& stream,
    const QString& text);

private:
  void setStatus(Status status);
  void openLogFiles();
  void closeLogFiles();
  void writeLogChunk(const QString& stream, const QString& text);
  static QString sanitizeName(const QString& name);

  QProcess process_;
  QString program_;
  QStringList arguments_;
  QString workingDirectory_;
  QString processName_ = "process";
  QString logDirectory_;
  QFile stdoutLog_;
  QFile stderrLog_;
  QFile combinedLog_;
  Status status_ = Status::Idle;
  QDateTime startedAt_;
  int exitCode_ = 0;
  QProcess::ExitStatus exitStatus_ = QProcess::NormalExit;
};
