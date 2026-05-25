#pragma once

#include <QAbstractTableModel>
#include <QDateTime>
#include <QString>
#include <QVector>

struct ProductRecord {
  QString filename;
  QString relativePath;
  QDateTime modifiedTime;
  qint64 sizeBytes = 0;
  QString fileType;
};

class ProductCatalogModel : public QAbstractTableModel {
  Q_OBJECT

public:
  explicit ProductCatalogModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QVariant headerData(
    int section,
    Qt::Orientation orientation,
    int role = Qt::DisplayRole) const override;

  void setProducts(const QVector<ProductRecord>& products);

private:
  QVector<ProductRecord> products_;
};
