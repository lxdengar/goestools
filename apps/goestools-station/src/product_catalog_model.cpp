#include "product_catalog_model.h"

#include <QLocale>

namespace {

enum Column {
  FilenameColumn,
  RelativePathColumn,
  ModifiedColumn,
  SizeColumn,
  TypeColumn,
  ColumnCount,
};

} // namespace

ProductCatalogModel::ProductCatalogModel(QObject* parent)
  : QAbstractTableModel(parent) {
}

int ProductCatalogModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return products_.size();
}

int ProductCatalogModel::columnCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return ColumnCount;
}

QVariant ProductCatalogModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= products_.size()) {
    return QVariant();
  }

  const auto& product = products_[index.row()];
  if (role == Qt::TextAlignmentRole && index.column() == SizeColumn) {
    return Qt::AlignRight;
  }

  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  switch (index.column()) {
  case FilenameColumn:
    return product.filename;
  case RelativePathColumn:
    return product.relativePath;
  case ModifiedColumn:
    return product.modifiedTime.toLocalTime().toString(Qt::ISODate);
  case SizeColumn:
    return QLocale().formattedDataSize(product.sizeBytes);
  case TypeColumn:
    return product.fileType;
  default:
    return QVariant();
  }
}

QVariant ProductCatalogModel::headerData(
    int section,
    Qt::Orientation orientation,
    int role) const {
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
    return QVariant();
  }

  switch (section) {
  case FilenameColumn:
    return "Filename";
  case RelativePathColumn:
    return "Relative path";
  case ModifiedColumn:
    return "Modified";
  case SizeColumn:
    return "Size";
  case TypeColumn:
    return "Type";
  default:
    return QVariant();
  }
}

void ProductCatalogModel::setProducts(const QVector<ProductRecord>& products) {
  beginResetModel();
  products_ = products;
  endResetModel();
}
