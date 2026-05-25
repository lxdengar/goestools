#include <QApplication>
#include <QAbstractItemView>
#include <QDateTime>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QPalette>
#include <QTabWidget>
#include <QTableView>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

#include "managed_process.h"
#include "product_catalog_model.h"
#include "product_scanner.h"

namespace {

QString now() {
  return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

void applyPageColor(QWidget* widget, const QString& color) {
  widget->setAutoFillBackground(true);
  auto palette = widget->palette();
  palette.setColor(QPalette::Window, QColor(color));
  widget->setPalette(palette);
}

QWidget* placeholderTab(const QString& label, const QString& color) {
  auto* widget = new QWidget;
  applyPageColor(widget, color);
  auto* layout = new QVBoxLayout(widget);
  auto* text = new QLabel(label);
  text->setAlignment(Qt::AlignCenter);
  layout->addWidget(text);
  return widget;
}

class ProcessTab : public QWidget {
public:
  explicit ProcessTab(ManagedProcess* process, QWidget* parent = nullptr)
    : QWidget(parent),
      process_(process) {
    applyPageColor(this, "#edf6ff");
    auto* layout = new QVBoxLayout(this);

    auto* commandRow = new QHBoxLayout;
    commandRow->addWidget(new QLabel("Command"));
    command_ = new QLineEdit;
    command_->setPlaceholderText("Path to external command");
    commandRow->addWidget(command_, 1);
    layout->addLayout(commandRow);

    auto* argumentsRow = new QHBoxLayout;
    argumentsRow->addWidget(new QLabel("Arguments"));
    arguments_ = new QLineEdit;
    arguments_->setPlaceholderText("Optional arguments");
    argumentsRow->addWidget(arguments_, 1);
    layout->addLayout(argumentsRow);

    auto* workdirRow = new QHBoxLayout;
    workdirRow->addWidget(new QLabel("Working directory"));
    workingDirectory_ = new QLineEdit;
    workingDirectory_->setPlaceholderText("Optional working directory");
    workdirRow->addWidget(workingDirectory_, 1);
    layout->addLayout(workdirRow);

    auto* controlsRow = new QHBoxLayout;
    startButton_ = new QPushButton("Start");
    stopButton_ = new QPushButton("Stop");
    restartButton_ = new QPushButton("Restart");
    stopButton_->setEnabled(false);
    restartButton_->setEnabled(false);
    status_ = new QLabel("Idle");
    controlsRow->addWidget(startButton_);
    controlsRow->addWidget(stopButton_);
    controlsRow->addWidget(restartButton_);
    controlsRow->addWidget(status_, 1);
    layout->addLayout(controlsRow);

    auto* detailsRow = new QHBoxLayout;
    pid_ = new QLabel("PID: -");
    uptime_ = new QLabel("Uptime: -");
    exitCode_ = new QLabel("Exit: -");
    detailsRow->addWidget(pid_);
    detailsRow->addWidget(uptime_);
    detailsRow->addWidget(exitCode_);
    detailsRow->addStretch(1);
    layout->addLayout(detailsRow);

    output_ = new QPlainTextEdit;
    output_->setReadOnly(true);
    output_->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(output_, 1);

    connect(startButton_, &QPushButton::clicked, this, [this] {
      startProcess();
    });
    connect(stopButton_, &QPushButton::clicked, this, [this] {
      stopProcess();
    });
    connect(restartButton_, &QPushButton::clicked, this, [this] {
      restartProcess();
    });
    connect(process_, &ManagedProcess::stdoutText, this, [this](const QString& text) {
      appendProcessOutput("stdout", text);
    });
    connect(process_, &ManagedProcess::stderrText, this, [this](const QString& text) {
      appendProcessOutput("stderr", text);
    });
    connect(process_,
            &ManagedProcess::started,
            this,
            [this](qint64 processId) {
              appendLine(QString("Started with PID %1.").arg(processId));
              refreshDetails();
            });
    connect(process_,
            &ManagedProcess::stopped,
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
              const auto message = exitStatus == QProcess::NormalExit
                ? QString("Finished with exit code %1.").arg(exitCode)
                : QString("Crashed.");
              appendLine(message);
              refreshDetails();
            });
    connect(process_, &ManagedProcess::failed, this, [this](const QString& error) {
      appendLine("Error: " + error);
      refreshDetails();
    });
    connect(process_,
            &ManagedProcess::statusChanged,
            this,
            [this](ManagedProcess::Status) {
              refreshDetails();
            });

    uptimeTimer_ = new QTimer(this);
    connect(uptimeTimer_, &QTimer::timeout, this, [this] {
      refreshDetails();
    });
    uptimeTimer_->start(1000);
  }

private:
  void configureProcess() {
    process_->configure(
      command_->text(),
      QProcess::splitCommand(arguments_->text()),
      workingDirectory_->text());
  }

  void startProcess() {
    output_->clear();
    configureProcess();
    appendLine("Starting: " + command_->text().trimmed() + " " + arguments_->text().trimmed());
    if (!process_->start()) {
      appendLine("Start request was not accepted.");
    }
  }

  void stopProcess() {
    appendLine("Stopping...");
    process_->stop();
  }

  void restartProcess() {
    configureProcess();
    appendLine("Restarting...");
    if (!process_->restart()) {
      appendLine("Restart request was not accepted.");
    }
  }

  void appendLine(const QString& line) {
    output_->appendPlainText("[" + now() + "] " + line);
  }

  void appendProcessOutput(const QString& stream, const QString& text) {
    output_->moveCursor(QTextCursor::End);
    output_->insertPlainText("[" + now() + "] " + stream + "\n");
    output_->insertPlainText(text);
    if (!text.endsWith('\n')) {
      output_->insertPlainText("\n");
    }
    output_->moveCursor(QTextCursor::End);
  }

  void refreshDetails() {
    const auto running = process_->status() == ManagedProcess::Status::Running
      || process_->status() == ManagedProcess::Status::Starting
      || process_->status() == ManagedProcess::Status::Stopping;

    startButton_->setEnabled(!running);
    stopButton_->setEnabled(running);
    restartButton_->setEnabled(running);
    status_->setText(process_->statusText());

    const auto processId = process_->pid();
    pid_->setText(processId > 0 ? QString("PID: %1").arg(processId) : "PID: -");
    uptime_->setText(
      process_->uptimeMs() > 0
        ? QString("Uptime: %1s").arg(process_->uptimeMs() / 1000)
        : "Uptime: -");

    if (process_->status() == ManagedProcess::Status::Finished) {
      exitCode_->setText(QString("Exit: %1").arg(process_->exitCode()));
    } else {
      exitCode_->setText("Exit: -");
    }
  }

  ManagedProcess* process_;
  QLineEdit* command_;
  QLineEdit* arguments_;
  QLineEdit* workingDirectory_;
  QPushButton* startButton_;
  QPushButton* stopButton_;
  QPushButton* restartButton_;
  QLabel* status_;
  QLabel* pid_;
  QLabel* uptime_;
  QLabel* exitCode_;
  QPlainTextEdit* output_;
  QTimer* uptimeTimer_;
};

class LogsTab : public QWidget {
public:
  explicit LogsTab(ManagedProcess* process, QWidget* parent = nullptr)
    : QWidget(parent),
      process_(process) {
    applyPageColor(this, "#fff4e8");
    auto* layout = new QVBoxLayout(this);

    auto* filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel("Process filter"));
    filter_ = new QLineEdit;
    filter_->setPlaceholderText("Leave empty to show all process output");
    filterRow->addWidget(filter_, 1);
    layout->addLayout(filterRow);

    logDirectory_ = new QLabel("Log directory: " + process_->logDirectory());
    layout->addWidget(logDirectory_);

    output_ = new QPlainTextEdit;
    output_->setReadOnly(true);
    output_->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(output_, 1);

    connect(filter_, &QLineEdit::textChanged, this, [this] {
      rebuildOutput();
    });
    connect(process_,
            &ManagedProcess::outputCaptured,
            this,
            [this](const QString& processName,
                   const QString& stream,
                   const QString& text) {
              LogEntry entry;
              entry.timestamp = now();
              entry.processName = processName;
              entry.stream = stream;
              entry.text = text;
              entries_.append(entry);

              if (accepts(entry)) {
                appendEntry(entry);
              }
            });
  }

private:
  struct LogEntry {
    QString timestamp;
    QString processName;
    QString stream;
    QString text;
  };

  bool accepts(const LogEntry& entry) const {
    const auto filter = filter_->text().trimmed();
    return filter.isEmpty()
      || entry.processName.contains(filter, Qt::CaseInsensitive);
  }

  void rebuildOutput() {
    output_->clear();
    for (const auto& entry : entries_) {
      if (accepts(entry)) {
        appendEntry(entry);
      }
    }
  }

  void appendEntry(const LogEntry& entry) {
    output_->moveCursor(QTextCursor::End);
    output_->insertPlainText(
      "[" + entry.timestamp + "] " + entry.processName + " " + entry.stream + "\n");
    output_->insertPlainText(entry.text);
    if (!entry.text.endsWith('\n')) {
      output_->insertPlainText("\n");
    }
    output_->moveCursor(QTextCursor::End);
  }

  ManagedProcess* process_;
  QLineEdit* filter_;
  QLabel* logDirectory_;
  QPlainTextEdit* output_;
  QVector<LogEntry> entries_;
};

class CatalogTab : public QWidget {
public:
  explicit CatalogTab(QWidget* parent = nullptr)
    : QWidget(parent),
      model_(new ProductCatalogModel(this)),
      scanner_(new ProductScanner(this)) {
    applyPageColor(this, "#f1f8ea");
    auto* layout = new QVBoxLayout(this);

    auto* controlsRow = new QHBoxLayout;
    controlsRow->addWidget(new QLabel("Output directory"));
    outputDirectory_ = new QLineEdit;
    outputDirectory_->setPlaceholderText("Directory containing generated products");
    scanButton_ = new QPushButton("Scan");
    controlsRow->addWidget(outputDirectory_, 1);
    controlsRow->addWidget(scanButton_);
    layout->addLayout(controlsRow);

    status_ = new QLabel("No scan run.");
    layout->addWidget(status_);

    table_ = new QTableView;
    table_->setModel(model_);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setSortingEnabled(true);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    layout->addWidget(table_, 1);

    connect(scanButton_, &QPushButton::clicked, this, [this] {
      scan();
    });
    connect(outputDirectory_, &QLineEdit::returnPressed, this, [this] {
      scan();
    });
  }

private:
  void scan() {
    const auto directory = outputDirectory_->text().trimmed();
    if (directory.isEmpty()) {
      status_->setText("Choose an output directory to scan.");
      model_->setProducts({});
      return;
    }

    const auto products = scanner_->scan(directory);
    model_->setProducts(products);
    table_->resizeColumnsToContents();
    status_->setText(
      QString("Found %1 products in %2.").arg(products.size()).arg(directory));
  }

  ProductCatalogModel* model_;
  ProductScanner* scanner_;
  QLineEdit* outputDirectory_;
  QPushButton* scanButton_;
  QLabel* status_;
  QTableView* table_;
};

class MainWindow : public QMainWindow {
public:
  MainWindow() {
    setWindowTitle("goestools Station");
    resize(1000, 700);

    auto* process = new ManagedProcess(this);

    auto* tabs = new QTabWidget;
    tabs->setStyleSheet(
      "QTabWidget::pane { border: 1px solid #b8bec8; }"
      "QTabBar::tab { padding: 8px 14px; color: #172033; font-weight: 600; }"
      "QTabBar::tab:nth-of-type(1) { background: #d7f2e3; }"
      "QTabBar::tab:nth-of-type(2) { background: #d8ebff; }"
      "QTabBar::tab:nth-of-type(3) { background: #efe1ff; }"
      "QTabBar::tab:nth-of-type(4) { background: #dff3d3; }"
      "QTabBar::tab:nth-of-type(5) { background: #ffe4c2; }"
      "QTabBar::tab:nth-of-type(6) { background: #f4e3cf; }"
      "QTabBar::tab:selected { border-bottom: 3px solid #172033; }"
    );

    tabs->addTab(placeholderTab("Receiver status will appear here.", "#ecfbf2"), "Status");
    tabs->addTab(new ProcessTab(process), "Processes");
    tabs->addTab(placeholderTab("Stats monitor will appear here.", "#f7edff"), "Monitor");
    tabs->addTab(new CatalogTab, "Catalog");
    tabs->addTab(new LogsTab(process), "Logs");
    tabs->addTab(placeholderTab("Station settings will appear here.", "#fbf0df"), "Settings");

    setCentralWidget(tabs);
  }
};

} // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  app.setApplicationName("goestools-station");
  MainWindow window;
  window.show();
  return app.exec();
}
