//  ***********************************************************************************************
//
//  libqcr: capture and replay Qt widget actions
//
//! @file      qcr/engine/qcrexception.h
//! @brief     Defines class QcrException
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2018-
//! @author    Joachim Wuttke
//
//  ***********************************************************************************************

#ifndef QCREXCEPTION_H
#define QCREXCEPTION_H

#include <QException>
#include <QString> // no auto rm

//! The sole exception type used in this software.
class QcrException : public QException {
public:
    QcrException() = delete;
    QcrException(const QString& msg) noexcept : msg_(msg) {}
    const QString& msg() const noexcept { return msg_; }
private:
    QString msg_;
};

#endif // QCREXCEPTION_H
