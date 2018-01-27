// ************************************************************************** //
//
//  Steca: stress and texture calculator
//
//! @file      gui/panels/subframe_setup.cpp
//! @brief     Implements class SubframeSetup, and local classes
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
// ************************************************************************** //

#include "gui/panels/subframe_setup.h"
#include "core/session.h"
#include "gui/base/table_model.h"
#include "gui/thehub.h"
#include "gui/base/tree_views.h" // inheriting from
#include "gui/base/new_q.h"

namespace {
qreal safeReal(qreal val) { return qIsFinite(val) ? val : 0.0; }
str safeRealText(qreal val) { return qIsFinite(val) ? str::number(val) : ""; }
} // local methods

// ************************************************************************** //
//  local class PeaksModel, used in PeaksView
// ************************************************************************** //

class PeaksModel : public TableModel {
public:
    PeaksModel() : TableModel() {}

    void addReflection(const QString& peakFunctionName);
    void removeReflection(int i);

    int columnCount() const final { return NUM_COLUMNS; }
    int rowCount() const final { return gSession->reflections().count(); }
    str displayData(int row, int col) const;
    str displayData(int row) const;
    QVariant data(const QModelIndex&, int) const;
    QVariant headerData(int, Qt::Orientation, int) const;

    enum { COL_ID = 1, COL_TYPE, NUM_COLUMNS };
};

void PeaksModel::addReflection(const QString& peakFunctionName) {
    gSession->addReflection(peakFunctionName);
    emit gSession->sigPeaksChanged();
}

void PeaksModel::removeReflection(int i) {
    gSession->removeReflection(i);
    if (gSession->reflections().isEmpty())
        gSession->peaks().select(nullptr);
    emit gSession->sigPeaksChanged();
}

str PeaksModel::displayData(int row, int col) const {
    switch (col) {
    case COL_ID:
        return str::number(row + 1);
    case COL_TYPE:
        return gSession->reflections().at(row)->peakFunction().name();
    default:
        NEVER return "";
    }
}

str PeaksModel::displayData(int row) const {
    return displayData(row, COL_ID) + ": " + displayData(row, COL_TYPE);
}

QVariant PeaksModel::data(const QModelIndex& index, int role) const {
    int row = index.row();
    if (row < 0 || rowCount() <= row)
        return {};
    switch (role) {
    case Qt::DisplayRole: {
        int col = index.column();
        if (col < 1)
            return {};
        switch (col) {
        case COL_ID:
        case COL_TYPE:
            return displayData(row, col);
        default:
            return {};
        }
    }
    default:
        return {};
    }
}

QVariant PeaksModel::headerData(int col, Qt::Orientation, int role) const {
    if (Qt::DisplayRole == role && COL_ID == col)
        return "#";
    return {};
}


// ************************************************************************** //
//  local class PeaksView
// ************************************************************************** //

class PeaksView final : public ListView {
public:
    PeaksView();

    void clear();
    void addReflection(const QString& peakFunctionName);
    void removeSelected();
    void updateSingleSelection();

    Reflection* selectedReflection() const;

private:
    void selectionChanged(QItemSelection const&, QItemSelection const&);
    PeaksModel* model_;
};

PeaksView::PeaksView() : ListView() {
    model_ = new PeaksModel();
    setModel(model_);
    for_i (model_->columnCount())
        resizeColumnToContents(i);
}

void PeaksView::clear() {
    for (int row = model_->rowCount(); row-- > 0;) {
        model_->removeReflection(row);
        updateSingleSelection();
    }
}

void PeaksView::addReflection(const QString& peakFunctionName) {
    model_->addReflection(peakFunctionName);
    updateSingleSelection();
}

void PeaksView::removeSelected() {
    int row = currentIndex().row();
    if (row < 0 || model_->rowCount() <= row)
        return;
    model_->removeReflection(row);
    updateSingleSelection();
}

void PeaksView::updateSingleSelection() {
    int row = currentIndex().row();
    model_->signalReset();
    setCurrentIndex(model_->index(row,0));
    gHub->trigger_removeReflection->setEnabled(model_->rowCount());
}

Reflection* PeaksView::selectedReflection() const { // TODO: is this needed ?
    QList<QModelIndex> indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty())
        return nullptr;
    int row = indexes.first().row();
    return gSession->reflections().at(row).data();
}

void PeaksView::selectionChanged(QItemSelection const& selected, QItemSelection const& deselected) {
    ListView::selectionChanged(selected, deselected);
    QList<QModelIndex> indexes = selected.indexes();
    gSession->peaks().select( indexes.isEmpty() ?
                              nullptr : gSession->reflections().at(indexes.first().row()).data());
}


// ************************************************************************** //
//  class ControlsPeakfits
// ************************************************************************** //

//! A widget with controls to view and change the detector geometry.

class ControlsPeakfits : public QWidget {
public:
    ControlsPeakfits();
private:
    void updateReflectionControls();
    void setReflControls();
    void newReflData(bool invalidateGuesses);

    class PeaksView* peaksView_;
    QComboBox* comboReflType_;
    QDoubleSpinBox *spinRangeMin_, *spinRangeMax_;
    QDoubleSpinBox *spinGuessPeakX_, *spinGuessPeakY_, *spinGuessFWHM_;
    QLineEdit *readFitPeakX_, *readFitPeakY_, *readFitFWHM_;

    bool silentSpin_ = false;
};

ControlsPeakfits::ControlsPeakfits() {

    auto* box = newQ::VBoxLayout();
    setLayout(box);

    QBoxLayout* hb = newQ::HBoxLayout();
    box->addLayout(hb);

    hb->addWidget(newQ::IconButton(gHub->toggle_selRegions));
    hb->addWidget(newQ::IconButton(gHub->toggle_showBackground));
    hb->addWidget(newQ::IconButton(gHub->trigger_clearPeaks));
    hb->addStretch();

    box->addWidget((peaksView_ = new PeaksView()));

    hb = newQ::HBoxLayout();
    box->addLayout(hb);

    comboReflType_ = new QComboBox;
    comboReflType_->addItems(FunctionRegistry::instance()->keys());
    hb->addWidget(comboReflType_);
    hb->addStretch();
    hb->addWidget(newQ::IconButton(gHub->trigger_addReflection));
    hb->addWidget(newQ::IconButton(gHub->trigger_removeReflection));

    QBoxLayout* vb = newQ::VBoxLayout();
    box->addLayout(vb);

    QGridLayout* gb = newQ::GridLayout();
    vb->addLayout(gb);

    gb->addWidget(newQ::Label("min"), 0, 0);
    gb->addWidget((spinRangeMin_ = newQ::DoubleSpinBox(6, true, .0)), 0, 1);
    spinRangeMin_->setSingleStep(.1);
    gb->addWidget(newQ::Label("max"), 0, 2);
    gb->addWidget((spinRangeMax_ = newQ::DoubleSpinBox(6, true, .0)), 0, 3);
    spinRangeMax_->setSingleStep(.1);

    gb->addWidget(newQ::Label("guess x"), 1, 0);
    gb->addWidget((spinGuessPeakX_ = newQ::DoubleSpinBox(6, true, .0)), 1, 1);
    spinGuessPeakX_->setSingleStep(.1);
    gb->addWidget(newQ::Label("y"), 1, 2);
    gb->addWidget((spinGuessPeakY_ = newQ::DoubleSpinBox(6, true, .0)), 1, 3);
    spinGuessPeakY_->setSingleStep(.1);

    gb->addWidget(newQ::Label("fwhm"), 2, 0);
    gb->addWidget((spinGuessFWHM_ = newQ::DoubleSpinBox(6, true, .0)), 2, 1);
    spinGuessFWHM_->setSingleStep(.1);

    gb->addWidget(newQ::Label("fit x"), 3, 0);
    gb->addWidget((readFitPeakX_ = newQ::LineDisplay(6, true)), 3, 1);
    gb->addWidget(newQ::Label("y"), 3, 2);
    gb->addWidget((readFitPeakY_ = newQ::LineDisplay(6, true)), 3, 3);

    gb->addWidget(newQ::Label("fwhm"), 4, 0);
    gb->addWidget((readFitFWHM_ = newQ::LineDisplay(6, true)), 4, 1);

    gb->setColumnStretch(4, 1);

    updateReflectionControls();

    connect(gHub->trigger_addReflection, &QAction::triggered, [this]() {
                peaksView_->addReflection(comboReflType_->currentText());
                updateReflectionControls();
            });

    connect(gHub->trigger_removeReflection, &QAction::triggered, [this]() {
                peaksView_->removeSelected();
                updateReflectionControls();
            });

    connect(gHub->trigger_clearPeaks, &QAction::triggered, [this]() {
                peaksView_->clear();
                updateReflectionControls();
            });

    connect(gSession, &Session::sigPeaksChanged, [this]() {
                peaksView_->updateSingleSelection();
                updateReflectionControls(); }
        );

    connect(comboReflType_, _SLOT_(QComboBox, currentIndexChanged, const QString&),
            [this](const QString& peakFunctionName) {
                if (gSession->peaks().selected_) { // TODO rm this if
                    gSession->peaks().selected_->setPeakFunction(peakFunctionName);
                    emit gSession->sigPeaksChanged();
                }
            });

    connect(gSession, &Session::sigReflectionSelected, this, &ControlsPeakfits::setReflControls);
    connect(gSession, &Session::sigReflectionData, this, &ControlsPeakfits::setReflControls);

    auto _changeReflData0 = [this](qreal /*unused*/) { newReflData(false); };
    auto _changeReflData1 = [this](qreal /*unused*/) { newReflData(true); };

    connect(spinRangeMin_, _SLOT_(QDoubleSpinBox, valueChanged, double), _changeReflData1);
    connect(spinRangeMax_, _SLOT_(QDoubleSpinBox, valueChanged, double), _changeReflData1);
    connect(spinGuessPeakX_, _SLOT_(QDoubleSpinBox, valueChanged, double), _changeReflData0);
    connect(spinGuessPeakY_, _SLOT_(QDoubleSpinBox, valueChanged, double), _changeReflData0);
    connect(spinGuessFWHM_, _SLOT_(QDoubleSpinBox, valueChanged, double), _changeReflData0);
}

void ControlsPeakfits::updateReflectionControls() {
    bool on = gSession->reflections().count();
    spinRangeMin_->setEnabled(on);
    spinRangeMax_->setEnabled(on);
    spinGuessPeakX_->setEnabled(on);
    spinGuessPeakY_->setEnabled(on);
    spinGuessFWHM_->setEnabled(on);
    readFitPeakX_->setEnabled(on);
    readFitPeakY_->setEnabled(on);
    readFitFWHM_->setEnabled(on);
};

void ControlsPeakfits::setReflControls() {
    Reflection* reflection = gSession->peaks().selected_;
    silentSpin_ = true;

    if (!reflection) {
        // do not set comboReflType - we want it to stay as it is
        spinRangeMin_->setValue(0);
        spinRangeMax_->setValue(0);
        spinGuessPeakX_->setValue(0);
        spinGuessPeakY_->setValue(0);
        spinGuessFWHM_->setValue(0);
        readFitPeakX_->clear();
        readFitPeakY_->clear();
        readFitFWHM_->clear();
    } else {
        {
            QSignalBlocker __(comboReflType_);
            comboReflType_->setCurrentText(reflection->peakFunctionName());
        }

        const Range& range = reflection->range();
        spinRangeMin_->setValue(safeReal(range.min));
        spinRangeMax_->setValue(safeReal(range.max));

        const PeakFunction& peakFun = reflection->peakFunction();
        const qpair& guessedPeak = peakFun.guessedPeak();
        spinGuessPeakX_->setValue(safeReal(guessedPeak.x));
        spinGuessPeakY_->setValue(safeReal(guessedPeak.y));
        spinGuessFWHM_->setValue(safeReal(peakFun.guessedFWHM()));

        const qpair& fittedPeak = peakFun.fittedPeak();
        readFitPeakX_->setText(safeRealText(fittedPeak.x));
        readFitPeakY_->setText(safeRealText(fittedPeak.y));
        readFitFWHM_->setText(safeRealText(peakFun.fittedFWHM()));
    }

    silentSpin_ = false;
};

void ControlsPeakfits::newReflData(bool invalidateGuesses) {
    if (!silentSpin_) {
        emit gSession->sigReflectionValues(
            Range::safeFrom(spinRangeMin_->value(), spinRangeMax_->value()),
            qpair(spinGuessPeakX_->value(), spinGuessPeakY_->value()),
            fwhm_t(spinGuessFWHM_->value()),
            invalidateGuesses);
    }
};


// ************************************************************************** //
//  class ControlsDetector
// ************************************************************************** //

//! A widget with controls to view and change the detector geometry.

class ControlsDetector : public QWidget {
public:
    ControlsDetector();
private:
    void toSession();
    void fromSession();

    QDoubleSpinBox *detDistance_, *detPixelSize_;
    QSpinBox *beamOffsetI_, *beamOffsetJ_;
    QSpinBox *cutLeft_, *cutTop_, *cutRight_, *cutBottom_;
};

ControlsDetector::ControlsDetector() {

    auto* box = newQ::VBoxLayout();
    setLayout(box);

    connect(gSession, &Session::sigDetector, this, &ControlsDetector::fromSession);

    // widgets

    detDistance_ = newQ::DoubleSpinBox(6, true, Geometry::MIN_DETECTOR_DISTANCE);
    detPixelSize_ = newQ::DoubleSpinBox(6, true, Geometry::MIN_DETECTOR_PIXEL_SIZE);
    detPixelSize_->setDecimals(3);

    detDistance_->setValue(Geometry::DEF_DETECTOR_DISTANCE);
    detPixelSize_->setValue(Geometry::DEF_DETECTOR_PIXEL_SIZE);

    connect(detDistance_, _SLOT_(QDoubleSpinBox, valueChanged, double),
            [this]() { toSession(); });
    connect(detPixelSize_, _SLOT_(QDoubleSpinBox, valueChanged, double),
            [this]() { toSession(); });

    beamOffsetI_ = newQ::SpinBox(6, true);
    beamOffsetJ_ = newQ::SpinBox(6, true);

//    connect(beamOffsetI_, _SLOT_(QSpinBox, valueChanged, int), [this]() { setToHub(); });

//    connect(beamOffsetJ_, _SLOT_(QSpinBox, valueChanged, int), [this]() { setToHub(); });

    cutLeft_ = newQ::SpinBox(4, false, 0);
    cutTop_ = newQ::SpinBox(4, false, 0);
    cutRight_ = newQ::SpinBox(4, false, 0);
    cutBottom_ = newQ::SpinBox(4, false, 0);

    auto _setImageCut = [this](bool isTopOrLeft, int value) {
        debug::ensure(value >= 0);
        if (gHub->toggle_linkCuts->isChecked())
            gSession->setImageCut(isTopOrLeft, true,
                                  ImageCut(value, value, value, value));
        else
            gSession->setImageCut(isTopOrLeft, false,
                                  ImageCut(cutLeft_->value(), cutTop_->value(),
                                           cutRight_->value(), cutBottom_->value()));
    };

    connect(cutLeft_, _SLOT_(QSpinBox, valueChanged, int), [_setImageCut](int value) {
            _setImageCut(true, value);
        });

    connect(cutTop_, _SLOT_(QSpinBox, valueChanged, int), [_setImageCut](int value) {
            _setImageCut(true, value);
        });

    connect(cutRight_, _SLOT_(QSpinBox, valueChanged, int), [_setImageCut](int value) {
            _setImageCut(false, value);
        });

    connect(cutBottom_, _SLOT_(QSpinBox, valueChanged, int), [_setImageCut](int value) {
            _setImageCut(false, value);
        });

    // layout

    QGridLayout* grid = newQ::GridLayout();
    int row = 0;

    auto _add = [&grid, &row](const QVector<QWidget*>& ws, int left = 1) {
        int i = 0, cnt = ws.count();
        QBoxLayout* box = newQ::HBoxLayout();
        box->addStretch(1);
        while (i < left)
            box->addWidget(ws.at(i++));
        grid->addLayout(box, row, 0);
        box = newQ::HBoxLayout();
        while (i < cnt)
            box->addWidget(ws.at(i++));
        grid->addLayout(box, row, 1);
        box->addStretch(1);
        row++;
    };

    _add({ newQ::Label("det. distance"),
                detDistance_,
                newQ::Label("mm") });
    _add({ newQ::Label("pixel size"),
                detPixelSize_,
                newQ::Label("mm") });
    _add({ newQ::Label("beam offset X"),
                beamOffsetI_,
                newQ::Label("pix") });
    _add({ newQ::Label("Y"),
                beamOffsetJ_,
                newQ::Label("pix") });
    _add({ newQ::Label("image rotate"),
                newQ::IconButton(gHub->trigger_rotateImage),
                newQ::Label("mirror"),
                newQ::IconButton(gHub->toggle_mirrorImage) });
    _add({ newQ::IconButton(gHub->toggle_linkCuts),
                newQ::Label("cut"),
                newQ::Icon(":/icon/cutLeft"),
                cutLeft_,
                newQ::Icon(":/icon/cutRight"),
                cutRight_ }, 3);
    _add({ newQ::Icon(":/icon/cutTop"),
                cutTop_,
                newQ::Icon(":/icon/cutBottom"),
                cutBottom_ });

    grid->setColumnStretch(grid->columnCount(), 1);

    box->addLayout(grid);
    box->addStretch();
}

void ControlsDetector::toSession() {
    gSession->setGeometry(
        qMax(qreal(Geometry::MIN_DETECTOR_DISTANCE), detDistance_->value()),
        qMax(qreal(Geometry::MIN_DETECTOR_PIXEL_SIZE), detPixelSize_->value()),
        IJ(beamOffsetI_->value(), beamOffsetJ_->value()));
}

void ControlsDetector::fromSession() {
    const Geometry& g = gSession->geometry();

    detDistance_->setValue(g.detectorDistance);
    detPixelSize_->setValue(g.pixSize);

    beamOffsetI_->setValue(g.midPixOffset.i);
    beamOffsetJ_->setValue(g.midPixOffset.j);

    const ImageCut& cut = gSession->imageCut();

    cutLeft_->setValue(cut.left);
    cutTop_->setValue(cut.top);
    cutRight_->setValue(cut.right);
    cutBottom_->setValue(cut.bottom);
}


// ************************************************************************** //
//  class ControlsBaseline
// ************************************************************************** //

//! A widget with controls to view and change the detector geometry.

class ControlsBaseline : public QWidget {
public:
    ControlsBaseline();
private:
    QSpinBox* spinDegree_;
};

ControlsBaseline::ControlsBaseline() {

    auto* box = newQ::VBoxLayout();
    setLayout(box);

    QBoxLayout* hb = newQ::HBoxLayout();
    box->addLayout(hb);

    hb->addWidget(newQ::IconButton(gHub->toggle_selRegions));
    hb->addWidget(newQ::IconButton(gHub->toggle_showBackground));
    hb->addWidget(newQ::IconButton(gHub->trigger_clearBackground));
    hb->addWidget(newQ::Label("Pol. degree:"));
    hb->addWidget((spinDegree_ =
                   newQ::SpinBox(4, false, 0, TheHub::MAX_POLYNOM_DEGREE)));
    hb->addStretch();

    box->addStretch(1);

    connect(spinDegree_, _SLOT_(QSpinBox, valueChanged, int), [this](int degree) {
            debug::ensure(degree >= 0);
            gSession->setBgPolyDegree(degree);
        });

    connect(gSession, &Session::sigBaseline, [this](){
            spinDegree_->setValue(gSession->bgPolyDegree()); });
}


// ************************************************************************** //
//  class SubframeSetup
// ************************************************************************** //

SubframeSetup::SubframeSetup() {
    setTabPosition(QTabWidget::North);

    addTab(new ControlsDetector(), "Detector");
    addTab(new ControlsBaseline(), "Baseline");
    addTab(new ControlsPeakfits(), "Peakfits");

    connect(this, &SubframeSetup::currentChanged, [this](int index) {
        eFittingTab tab;
        if (index==1)
            tab = eFittingTab::BACKGROUND;
        else if (index==2)
            tab = eFittingTab::REFLECTIONS;
        else
            tab = eFittingTab::NONE;
        gHub->setFittingTab(tab);
    });

    gHub->setFittingTab(eFittingTab::NONE);
}
