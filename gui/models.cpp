// ************************************************************************** //
//
//  STeCa2:    StressTexCalculator ver. 2
//
//! @file      models.cpp
//!
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016
//! @authors   Scientific Computing Group at MLZ Garching
//! @authors   Original version: Christian Randau
//! @authors   Version 2: Antti Soininen, Jan Burle, Rebecca Brydon
//
// ************************************************************************** //

#include "models.h"
#include "thehub.h"
#include <QCheckBox>

namespace models {
//------------------------------------------------------------------------------

FileViewModel::FileViewModel(gui::TheHub& hub): TableModel(hub) {
}

int FileViewModel::columnCount(rcIndex) const {
  return 1 + DCOL;
}

int FileViewModel::rowCount(rcIndex) const {
  return hub_.numFiles();
}

QVariant FileViewModel::data(rcIndex index,int role) const {
  auto row = index.row(), rowCnt = rowCount();
  if (row < 0 || rowCnt <= row) return EMPTY_VAR;

  switch (role) {
    case Qt::DisplayRole:
      return hub_.fileName(row);
    case GetFileRole:
      return QVariant::fromValue<core::shp_File>(hub_.getFile(row));
    default:
      return EMPTY_VAR;
  }
}

void FileViewModel::remFile(uint i) {
  hub_.remFile(i);
}

//------------------------------------------------------------------------------

DatasetViewModel::DatasetViewModel(gui::TheHub& hub)
: super(hub), file_(nullptr), metaInfo_(nullptr) {
}

int DatasetViewModel::columnCount(rcIndex) const {
  return COL_ATTRS + metaInfoNums_.count();
}

int DatasetViewModel::rowCount(rcIndex) const {
  return file_ ? file_->datasets().count() : 0;
}

QVariant DatasetViewModel::data(rcIndex index,int role) const {
  if (!file_) return EMPTY_VAR;

  int row = index.row();
  if (row < 0 || rowCount() <= row) return EMPTY_VAR;

  switch (role) {
  case Qt::DisplayRole: {
    int col = index.column();
    if (col < DCOL || columnCount() <= col) return EMPTY_VAR;

    switch (col) {
    case COL_NUMBER:
      return str::number(row+1);
    default:
      return file_->datasets().at(row)->attributeStrValue(metaInfoNums_[col-COL_ATTRS]);
    }
  }

  case GetDatasetRole:
    return QVariant::fromValue<core::shp_Dataset>(file_->datasets().at(row));
  default:
    return EMPTY_VAR;
  }
}

QVariant DatasetViewModel::headerData(int col, Qt::Orientation, int role) const {
  if (Qt::DisplayRole != role || col < DCOL || columnCount() <= col)
    return EMPTY_VAR;

  switch (col) {
  case COL_NUMBER:
    return "#";
  default:
    return core::Dataset::attributeTag(metaInfoNums_[col-COL_ATTRS]);
  }
}

void DatasetViewModel::setFile(core::shp_File file) {
  beginResetModel();
  file_ = file;
  endResetModel();
}

void DatasetViewModel::showMetaInfo(checkedinfo_vec const& infos_) {
  beginResetModel();

  metaInfoNums_.clear();

  if ((metaInfo_ = &infos_)) {
    for_i (metaInfo_->count()) {
      auto &item = metaInfo_->at(i);
      if (item.cb->isChecked())
        metaInfoNums_.append(i);
    }
  }

 endResetModel();
}

//------------------------------------------------------------------------------

ReflectionViewModel::ReflectionViewModel(gui::TheHub& hub): super(hub) {
}

int ReflectionViewModel::columnCount(rcIndex) const {
  return NUM_COLUMNS;
}

int ReflectionViewModel::rowCount(rcIndex) const {
  return hub_.reflections().count();
}

QVariant ReflectionViewModel::data(rcIndex index, int role) const {
  int row = index.row();
  if (row < 0 || rowCount() <= row) return EMPTY_VAR;

  switch (role) {
  case Qt::DisplayRole: {
    int col = index.column();
    if (col < DCOL) return EMPTY_VAR;

    switch (col) {
    case COL_ID:
      return str::number(row+1);
    case COL_TYPE:
      return core::Reflection::typeTag(hub_.reflections()[row]->type());
    default:
      return EMPTY_VAR;
    }
  }

  case GetDatasetRole:
    return QVariant::fromValue<core::shp_Reflection>(hub_.reflections()[row]);
  default:
    return EMPTY_VAR;
  }
}

QVariant ReflectionViewModel::headerData(int col, Qt::Orientation, int role) const {
  if (Qt::DisplayRole == role && COL_ID == col)
    return "#";
  else
    return EMPTY_VAR;
}

void ReflectionViewModel::addReflection(core::ePeakType type) {
  hub_.addReflection(type);
}

void ReflectionViewModel::remReflection(uint i) {
  hub_.remReflection(i);
}

//------------------------------------------------------------------------------
}
// eof
