//  ***********************************************************************************************
//
//  Steca: stress and texture calculator
//
//! @file      gui/dialogs/load_data.cpp
//! @brief     Implements functions in namespace loadData
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
//  ***********************************************************************************************

#include "gui/dialogs/load_data.h"
#include "core/session.h"
#include "core/base/async.h"
#include "core/base/exception.h"
#include "gui/dialogs/file_dialog.h"
#include "qcr/base/debug.h" // warning
#include <QDir>
#include <QStringBuilder> // for ".." % ..

namespace {
QDir dataDir_ {QDir::homePath()};
const QString dataFormats {"Data files (*.dat *.yaml *.mar*);;All files (*.*)"};
} // namespace

void loadData::addFiles(QWidget* parent)
{
    file_dialog::queryImportFileNames(
        parent, "Add files", dataDir_, dataFormats, true,
        [](QStringList fileNames){
            qDebug() << "DEBUG loadData::addFiles postprocess" << fileNames;
            if (fileNames.isEmpty())
                return;
            TakesLongTime __("addFiles");
            try {
                gSession->dataset.addGivenFiles(fileNames);
            } catch (const Exception& ex) {
                qWarning() << ex.msg();
            }
        });
}

void loadData::loadCorrFile(QWidget* parent)
{
    if (gSession->corrset.hasFile()) {
        gSession->corrset.removeFile();
        return;
    }
    file_dialog::queryImportFileNames(
        parent, "Set correction file", dataDir_, dataFormats, false,
        [](QStringList fileNames){
            if (fileNames.isEmpty())
                return;
            try {
                gSession->corrset.loadFile(fileNames.first());
            } catch (const Exception& ex) {
                qWarning() << ex.msg();
            }
        });
}
