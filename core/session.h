//  ***********************************************************************************************
//
//  Steca: stress and texture calculator
//
//! @file      core/session.h
//! @brief     Defines class Session
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
//  ***********************************************************************************************

#ifndef SESSION_H
#define SESSION_H

#include "core/calc/active_clusters.h"
#include "core/calc/allpeaks_allinfos.h"
#include "core/data/corrset.h"
#include "core/data/dataset.h"
#include "core/data/gamma_selection.h"
#include "core/data/theta_selection.h"
#include "core/pars/baseline.h"
#include "core/pars/params.h"
#include "core/pars/allpeaks_settings.h"
#include "core/data/angle_map.h"
#include "core/typ/lazy_data.h"

extern class Session* gSession;

//! Holds data and data-related settings.
//! Singleton, accessible from everywhere through the global pointer gSession.

class Session {
public:
    Session();
    Session(const Session&) = delete;

    // modifying methods:
    void clear();
    void sessionFromJson(const QByteArray&);

    void updateImageSize();           //!< Clears image size if session has no files
    void setImageSize(const size2d&); //!< Also ensures same size for all images

    void onDetector() const;      //!< detector detector has changed
    void onCut() const;           //!< image cuts have changed
    void onBaseline() const;      //!< settings for baseline fit have changed
    void onPeaks() const;         //!< a peak has been added or removed
    void onInterpol() const;      //!< interpolation control parameters have changed
    void onNormalization() const; //!< normalization parameters have changed

    // const methods:
    QByteArray serializeSession() const; // TODO rename toJson
    size2d imageSize() const;

    QStringList allAsciiKeys() const;
    QStringList allNiceKeys() const;
    QStringList numericAsciiKeys() const;
    QStringList numericNiceKeys() const;
    bool hasSigma(int index) const;

    // const abbreviations to member member calls
    bool hasData() const { return dataset.countFiles(); }
    bool hasCorrFile() const { return corrset.hasFile(); }
    const Cluster* currentCluster() const { return dataset.highlight().cluster(); }
    const Dfgram* currentOrAvgeDfgram() const;

    AllPeaksAllInfos peaksOutcome;      //!< all the outcome of peak raw analysis or fitting
    /* order matters for destruction... */
    Dataset dataset;                    //!< raw data files with sample detector images
    Corrset corrset;                    //!< raw data files with standard sample image
    Params  params;                     //!< global parameters like detector geometry, ...
    GammaSelection gammaSelection; // TODO reconsider
    ThetaSelection thetaSelection; // TODO reconsider
    Baseline baseline;                  //!< ranges and other parameters for baseline fitting
    AllPeaksSettings peaksSettings;     //!< ranges and other parameters for Bragg peak fitting
    ActiveClusters activeClusters;      //!< list of all clusters except the unselected ones
    lazy_data::KeyedCache<AngleMap,deg> angleMap; //!< to accelerate the projection image->dfgram

private:
    size2d imageSize_; //!< All images must have this same size
};

#endif // SESSION_H
