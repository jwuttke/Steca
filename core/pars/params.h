//  ***********************************************************************************************
//
//  Steca: stress and texture calculator
//
//! @file      core/pars/params.h
//! @brief     Defines class Params
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
//  ***********************************************************************************************

#ifndef PARAMS_H
#define PARAMS_H

#include "core/pars/detector.h"
#include "core/pars/image_transform.h"
#include "core/pars/interpol_params.h"
#include "core/typ/bool_vector.h"
#include "qcr/engine/cell.h"
#include "qcr/engine/enum_cell.h"

enum class eNorm {
    NONE,
    MONITOR,
    DELTA_MONITOR,
    TIME,
    DELTA_TIME,
};

//! Global user-selected parameters

class Params {
public:
    Params() {}
    Params(const Params&) = delete;
    // void clear() { *this = {}; } TODO restore (broken because BoolVector disallows copying

    void onMeta(); //!< To be called when list of meta data has changed.

    Detector        detector;
    ImageTransform  imageTransform;
    ImageCut        imageCut;
    InterpolParams  interpolParams;
    QcrCell<bool>   intenScaledAvg {true}; // if not, summed
    QcrCell<double> intenScale {1.};
    eNorm           normMode {eNorm::NONE};
    BoolVector      smallMetaSelection;  //!< for 'clusters' and 'metadata' subframes:
    BoolVector      bigMetaSelection;    //! for use in 'bigtable' (tabbed view and export):
    QcrEnumCell     diagramX;            //!< for use as x axis in diagram
    QcrEnumCell     diagramY;            //!< for use as y axis in diagram
};

#endif // PARAMS_H
