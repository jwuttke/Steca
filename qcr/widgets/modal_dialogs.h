//  ***********************************************************************************************
//
//  libqcr: capture and replay Qt widget actions
//
//! @file      qcr/widgets/modal_dialogs.h
//! @brief     Defines classes QcrModal, QcrModalDialog, QcrFileDialog
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2018-
//! @author    Joachim Wuttke
//
//  ***********************************************************************************************

#ifndef MODAL_DIALOGS_H
#define MODAL_DIALOGS_H

#include "qcr/engine/mixin.h"
#include <QFileDialog>

//! Mix-in for modal (blocking) dialogs.
class QcrModal : public QcrCommandable {
protected:
    QcrModal(const QString& name);
    ~QcrModal();
};

//! Dialog, for modal use.
class QcrModalDialog : protected QcrModal, public QDialog {
public:
    QcrModalDialog(QWidget* parent, const QString& caption);
    void setFromCommand(const QString&) override;
};

//! File dialog, for modal use, with console commands to select files and to close the dialog.
class QcrFileDialog : protected QcrModal, public QFileDialog {
public:
    QcrFileDialog(QWidget* parent, const QString& caption, const QString& directory,
                  const QString& filter, std::function<void(const QStringList)> postprocess);
    ~QcrFileDialog();
    void setFromCommand(const QString&) override;
};

#endif // MODAL_DIALOGS_H
