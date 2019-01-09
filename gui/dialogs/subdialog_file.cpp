//  ***********************************************************************************************
//
//  Steca: stress and texture calculator
//
//! @file      gui/dialogs/subdialog_file.cpp
//! @brief     Implements classes ExportfileDialogfield
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
//  ***********************************************************************************************

#include "gui/dialogs/subdialog_file.h"
#include "gui/dialogs/file_dialog.h"
//#include "qcr/base/debug.h"
#include <QGroupBox>
#include <qmath.h>

//! Implementation of numberedFileName in header.
QString numberedFileName(const QString& templatedName, int num, int maxNum) {
    if (!templatedName.contains("%d"))
        qFatal("path does not contain placeholder %%d");
    QString ret = templatedName;
    int nDigits = (int)log10((double)maxNum)+1;
    ret.replace("%d", QString("%1").arg(num, nDigits, 10, QLatin1Char('0')));
    return ret;
}

namespace {
static QString saveFmt = "dat"; //!< setting: default format for data export
} // namespace

ExportfileDialogfield::ExportfileDialogfield(
    QWidget* parent, bool withTypes, std::function<void(void)> onSave)
{
    progressBar.hide();

    static QDir defaultDir = QDir::homePath();

    dir_ = new QcrLineEdit("dir", defaultDir.absolutePath());
    file_ = new QcrLineEdit("file");

    auto* fileExtensionGroup = new QButtonGroup;
    auto* ftypeGrid = new QVBoxLayout;
    for (const QString fmt: { "dat", "csv" }) {
        auto* rb = new QcrRadioButton{"fmt."+fmt, "."+fmt};
        rb->programaticallySetValue(saveFmt == fmt);
        connect(rb, &QRadioButton::clicked, [fmt]() { saveFmt == fmt; });
        fileExtensionGroup->addButton(rb);
        ftypeGrid->addWidget(rb);
    }

    dir_->setReadOnly(true);

    auto* actBrowse_ = new QcrTrigger{"selectDir", "Browse..."};
    auto* actCancel_ = new QcrTrigger{"cancel", "Cancel"};
    auto* actSave_   = new QcrTrigger{"save", "Save"};

    // internal connections
    connect(actBrowse_, &QAction::triggered, [this, parent]() {
            dir_->setText(file_dialog::queryDirectory(parent, "Select folder", dir_->text())); });

    // outgoing connections
    connect(actCancel_, &QAction::triggered, [parent]() { parent->close(); });
    connect(actSave_, &QAction::triggered, onSave);

    // layout
    auto* destinationGrid = new QGridLayout;
    destinationGrid->addWidget(new QLabel("Save to folder:"), 0, 0, Qt::AlignRight);
    destinationGrid->addWidget(dir_,                          0, 1);
    destinationGrid->addWidget(new QcrTextTriggerButton(actBrowse_),0, 2);
    destinationGrid->addWidget(new QLabel("File name:"),      1, 0, Qt::AlignRight);
    destinationGrid->addWidget(file_,                         1, 1);

    auto* destination = new QGroupBox("Destination");
    destination->setLayout(destinationGrid);

    auto* ftype = new QGroupBox("File type");
    ftype->setVisible(withTypes);
    ftype->setLayout(ftypeGrid);

    auto* setup = new QHBoxLayout;
    setup->addWidget(destination);
    setup->addWidget(ftype);

    auto* bottom = new QHBoxLayout();
    bottom->addWidget(&progressBar);
    bottom->setStretchFactor(&progressBar, 333);
    bottom->addStretch(1);
    bottom->addWidget(new QcrTextTriggerButton(actCancel_));
    bottom->addWidget(new QcrTextTriggerButton(actSave_));

    addLayout(setup);
    addLayout(bottom);
}

QString ExportfileDialogfield::path(bool withSuffix, bool withNumber)
{
    QString dir = dir_->text().trimmed();
    QString fileName = file_->text().trimmed();
    if (dir.isEmpty() || fileName.isEmpty())
        return "";
    if (withNumber && !fileName.contains("%d"))
        fileName += ".%d";
    if (withSuffix) {
        QString suffix = saveFmt;
        if ("."+QFileInfo(fileName).suffix().toLower()!=saveFmt.toLower())
            fileName += "."+saveFmt;
    }
    file_->setText(fileName);

    return QFileInfo(dir + '/' + fileName).absoluteFilePath();
}

QFile* ExportfileDialogfield::file()
{
    QString tmp = path(true);
    if (tmp.isEmpty())
        return {};
    return file_dialog::openFileConfirmOverwrite("file", parentWidget(), tmp);

}

QString ExportfileDialogfield::separator() const
{
    if      (saveFmt=="dat")
        return " ";
    else if (saveFmt=="csv")
        return ", ";
    else
        qFatal("invalid case in ExportfileDialogfield::separator");
}
