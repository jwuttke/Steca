//  ***********************************************************************************************
//
//  Steca: stress and texture calculator
//
//! @file      gui/mainwin.cpp
//! @brief     Implements class MainWin
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
//  ***********************************************************************************************

#include "gui/mainwin.h"
#include "core/base/async.h"
#include "core/session.h"
#include "gui/actions/image_trafo_actions.h"
#include "gui/actions/menus.h"
#include "gui/view/toggles.h"
#include "gui/actions/triggers.h"
#include "gui/panels/mainframe.h"
#include "gui/panels/subframe_clusters.h"
#include "gui/panels/subframe_dfgram.h"
#include "gui/panels/subframe_files.h"
#include "gui/panels/subframe_metadata.h"
#include "gui/panels/subframe_setup.h"
#include "QCR/engine/console.h"
#include "QCR/engine/logger.h"
//#include "QCR/base/debug.h"
#include <QApplication>
#include <QProgressBar>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTimer>

MainWin* gGui; //!< global pointer to _the_ main window

//  ***********************************************************************************************
//! @class MainWin

MainWin::MainWin(const QString& startupScript)
{
    gGui = this;

    triggers = new Triggers;
    toggles = new Toggles;
    imageTrafoActions = new ImageTrafoActions;
    menus_ = new Menus(menuBar());

    setWindowIcon(QIcon{":/icon/retroStier"});
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    setAttribute(Qt::WA_DeleteOnClose, true);

    // layout
    setContentsMargins(5,5,5,5);

    addDockWidget(Qt::LeftDockWidgetArea, (dockFiles_    = new SubframeFiles));
    addDockWidget(Qt::LeftDockWidgetArea, (dockClusters_ = new SubframeClusters));
    addDockWidget(Qt::LeftDockWidgetArea, (dockMetadata_ = new SubframeMetadata));

    auto* splTop = new QSplitter{Qt::Horizontal};
    splTop->setChildrenCollapsible(false);
    splTop->addWidget(new SubframeSetup);
    splTop->addWidget(new Mainframe);
    splTop->setStretchFactor(1, 1);

    auto* splMain = new QSplitter{Qt::Vertical};
    splMain->setChildrenCollapsible(false);
    splMain->addWidget(splTop);
    splMain->addWidget(new SubframeDfgram);
    splMain->setStretchFactor(1, 1);
    setCentralWidget(splMain);

    auto* progressBar = new QProgressBar{this};
    statusBar()->addWidget(progressBar);
    TakesLongTime::registerProgressBar(progressBar);

    toggles->viewStatusbar.setHook([this](bool on){statusBar()  ->setVisible(on);});
    toggles->viewFiles    .setHook([this](bool on){dockFiles_   ->setVisible(on);});
    toggles->viewClusters .setHook([this](bool on){dockClusters_->setVisible(on);});
    toggles->viewMetadata .setHook([this](bool on){dockMetadata_->setVisible(on);});
#ifndef Q_OS_OSX
    toggles->fullScreen   .setHook([this](bool on){
            if (on) showFullScreen(); else showNormal();});
#endif

    readSettings();

    setRemake( [=]() { refresh(); } );
    show(); // must be called before initial remakeAll because remakeAll depends on visibility
    remakeAll();
    gLogger->setCaller("gui");

    if (startupScript!="")
        // delay execution until hopefully this MainWin is shown
        QTimer::singleShot(25, qApp, [=](){ gConsole->runScript(startupScript); });
}

MainWin::~MainWin()
{
    saveSettings();
    delete imageTrafoActions;
    delete triggers;
    delete toggles;
    delete menus_;
    gGui = nullptr;
}

void MainWin::refresh()
{
    bool hasData = gSession->hasData();
    bool hasPeak = gSession->peaksSettings.size();
    bool hasBase = gSession->baseline.ranges.size();
    toggles->enableCorr.setEnabled(gSession->hasCorrFile());
    triggers->exportDfgram.setEnabled(hasData);
    triggers->exportBigtable.setEnabled(hasData && hasPeak);
    triggers->exportDiagram.setEnabled(hasData && hasPeak);
    triggers->baserangeAdd   .setEnabled(hasData);
    triggers->baserangeRemove.setEnabled(hasBase);
    triggers->baserangesClear.setEnabled(hasBase);
    triggers->peakAdd   .setEnabled(hasData);
    triggers->peakRemove.setEnabled(hasPeak);
    triggers->peaksClear.setEnabled(hasPeak);
    triggers->removeFile.setEnabled(hasData);
    triggers->clearFiles.setEnabled(hasData);
    menus_->export_->setEnabled(hasData);
    menus_->image_->setEnabled(hasData);
    menus_->dgram_->setEnabled(hasData);
}

void MainWin::resetViews()
{
    restoreState(initialState_);
#ifndef Q_OS_OSX
    toggles->fullScreen.setCellValue(false);
#endif
    toggles->viewStatusbar.setCellValue(true);
    toggles->viewClusters.setCellValue(true);
    toggles->viewFiles.setCellValue(true);
    toggles->viewMetadata.setCellValue(true);
}

//! Stores native defaults as initialState_, then reads from config file.
void MainWin::readSettings()
{
    if (initialState_.isEmpty())
        initialState_ = saveState();
    QSettings s;
    s.beginGroup("MainWin");
    restoreGeometry(s.value("geometry").toByteArray());
    restoreState(s.value("state").toByteArray());
}

void MainWin::saveSettings() const
{
    QSettings s;
    s.beginGroup("MainWin");
    s.setValue("geometry", saveGeometry()); // this mainwindow's widget geometry
    s.setValue("state", saveState()); // state of this mainwindow's toolbars and dockwidgets
}
