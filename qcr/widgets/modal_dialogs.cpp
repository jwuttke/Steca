//  ***********************************************************************************************
//
//  libqcr: capture and replay Qt widget actions
//
//! @file      qcr/widgets/modal_dialogs.cpp
//! @brief     Implements classes QcrModal, QcrModalDialog, QcrFileDialog
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2018-
//! @author    Joachim Wuttke
//
//  ***********************************************************************************************

#include "qcr/widgets/modal_dialogs.h"
#include "qcr/base/debug.h"
#include "qcr/base/qcrexception.h"
#include "qcr/engine/console.h"


//  ***********************************************************************************************
//! @class QcrModal

QcrModal::QcrModal(const QString& name)
    : QcrCommandable {gConsole->learn("@push " + name,this)}
{}

QcrModal::~QcrModal()
{
    ASSERT(preclosed_);
}

//! @brief To be called when `*this` closes.
//! Logs `accept` or `cancel` events. Removes `*this` from Console registry.

//! In an ideal world, the cleanup functionality of `preclose` would be implemented in
//! the destructor `~QcrModal`. However, that destructor is not immediately called upon
//! `close`. Rather, `close` seems to call `deleteLater` which returns control to the
//! main event loop from where ultimately the destructor is called (see also
//! https://stackoverflow.com/questions/55068425). This delay is incompatible with
//! replay mode: the replay engine would attempt to execute the next main window command,
//! and fail, because QcrModal has not yet removed itself from the top of the command registry.
//!
//! Therefore, it is imperative that `preclose` be called as soon as there is an `accept`
//! or `reject` event. To detect violations of this rule, there are assertions on the
//! boolean variable `preclosed_`.

void QcrModal::preclose(int result)
{
    ASSERT(!preclosed_);
    gConsole->log(name() + " " + (result==QDialog::Accepted ? "accept" : "cancel"));
    gConsole->forget(name());
    gConsole->closeModalDialog(name());
    preclosed_ = true;
}


//  ***********************************************************************************************
//! @class QcrModalDialog

QcrModalDialog::QcrModalDialog(QWidget* parent, const QString& caption)
    : QcrModal{"modal"}
    , QDialog {parent}
{
    setWindowTitle(caption);
    setModal(true);
    setAttribute(Qt::WA_DeleteOnClose, true);
}

void QcrModalDialog::setFromCommand(const QString& arg)
{
    if (arg=="")
        throw QcrException{"Empty argument in Dialog command"};
    if (arg=="close") {
        accept();
        return;
    }
}


//  ***********************************************************************************************
//! @class QcrFileDialog

QcrFileDialog::QcrFileDialog(
    QWidget* parent, const QString& caption, const QString& directory, const QString& filter,
    std::function<void(const QStringList)> postprocess)
    : QcrModal{"fdia"}
    , QFileDialog{parent, caption, directory, filter}
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    connect(this, &QcrFileDialog::finished, this,
            [this,postprocess](){
                if (result()==Accepted)
                    postprocess(this->selectedFiles());
                preclose(result());
                close();
            });
    connect(this, &QcrFileDialog::filesSelected, this,
            [this](const QStringList& selected){
                gConsole->log("fdia select "+selectedFiles().join(';')); });
}

void QcrFileDialog::setFromCommand(const QString& arg)
{
    if (arg=="")
        throw QcrException{"Empty argument in FileDialog command"};
    QStringList args = arg.split(' ');
    QString cmd = args[0];
    if      (cmd=="accept")
        accept(); // will emit signal finished(), which triggers postprocess and close
    else if (cmd=="cancel")
        reject();
    else if (cmd=="select") {
        if (args.size()<2)
            throw QcrException{"Missing argument to command 'select'"};
        QStringList list = args[1].split(';');
        QString tmp = '"' + list.join("\" \"") + '"';
        selectFile(tmp);
    } else
        throw QcrException{"Unexpected filedialog command "+arg};
}
