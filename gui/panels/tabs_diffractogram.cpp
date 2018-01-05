// ************************************************************************** //
//
//  Steca2: stress and texture calculator
//
//! @file      gui/panels/tabs_diffractogram.cpp
//! @brief     Implements class TabsDiffractogram
//!
//! @homepage  https://github.com/scgmlz/Steca2
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
// ************************************************************************** //

#include "tabs_diffractogram.h"
#include "panel_diffractogram.h"
#include <QBoxLayout>

TabsDiffractogram::TabsDiffractogram() {
    setTabPosition(QTabWidget::North);
    wmaker::newTab(this, "Diffractogram")->box().addWidget(new Diffractogram());
}
