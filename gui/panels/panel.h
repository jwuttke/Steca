// ************************************************************************** //
//
//  Steca2: stress and texture calculator
//
//! @file      gui/panels/panel.h
//! @brief     Defines classes Tab, TabsPanel
//!
//! @homepage  https://github.com/scgmlz/Steca2
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
// ************************************************************************** //

#ifndef PANEL_H
#define PANEL_H

#include "typ/str.h"
#include <QTabWidget>

class QBoxLayout;

//! A tabbed panel
class PTab : public QWidget {
public:
    PTab(Qt::Orientation);
    QBoxLayout& box() const { return *box_; }

protected:
    QBoxLayout* box_;
};

namespace wmaker {
PTab* newTab(QTabWidget* panel, rcstr title);
}

#endif // PANEL_H
