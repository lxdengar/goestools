#include "product_scanner.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>

#include <algorithm>

ProductScanner::ProductScanner(QObject* parent)
  : QObject(parent) {
}

QVector<ProductRecord> ProductScanner::scan(const QString& outputDirectory) const {
  QVector<ProductRecord> products;
  const QDir root(outputDirectory);
  if (!root.exists()) {
    return products;
  }

  QDirIterator it(
    root.absolutePath(),
    QDir::Files | QDir::NoSymLinks,
    QDirIterator::Subdirectories);

  while (it.hasNext()) {
    const auto path = it.next();
    const QFileInfo info(path);

    ProductRecord product;
    product.filename = info.fileName();
    product.relativePath = root.relativeFilePath(info.absoluteFilePath());
    product.modifiedTime = info.lastModified().toUTC();
    product.sizeBytes = info.size();
    product.fileType = inferFileType(info.absoluteFilePath());
    products.append(product);
  }

  std::sort(products.begin(), products.end(), [](const auto& a, const auto& b) {
    return a.modifiedTime > b.modifiedTime;
  });

  return products;
}

QString ProductScanner::inferFileType(const QString& path) {
  const QFileInfo info(path);
  const auto suffix = info.suffix().toLower();
  const auto lowerPath = path.toLower();

  const QSet<QString> imageTypes = {"png", "jpg", "jpeg", "gif", "tif", "tiff"};
  const QSet<QString> textTypes = {"txt", "text", "md", "html", "xml", "json", "csv"};
  const QSet<QString> archiveTypes = {"zip", "gz", "bz2", "xz"};

  if (lowerPath.contains("emwin")) {
    return "EMWIN";
  }
  if (lowerPath.contains("dcs")) {
    return "DCS";
  }
  if (imageTypes.contains(suffix)) {
    return "Image";
  }
  if (textTypes.contains(suffix)) {
    return "Text";
  }
  if (suffix == "raw") {
    return "Packet capture";
  }
  if (suffix.startsWith("lrit") || lowerPath.contains(".lrit")) {
    return "LRIT";
  }
  if (archiveTypes.contains(suffix)) {
    return "Archive";
  }
  return "Unknown";
}
