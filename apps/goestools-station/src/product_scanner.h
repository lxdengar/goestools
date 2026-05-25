#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "product_catalog_model.h"

class ProductScanner : public QObject {
  Q_OBJECT

public:
  explicit ProductScanner(QObject* parent = nullptr);

  QVector<ProductRecord> scan(const QString& outputDirectory) const;

private:
  static QString inferFileType(const QString& path);
};
