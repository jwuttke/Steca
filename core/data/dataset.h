// ************************************************************************** //
//
//  Steca2: stress and texture calculator
//
//! @file      core/data/dataset.h
//! @brief     Defines classes [One]Dataset[s]
//!
//! @homepage  https://github.com/scgmlz/Steca2
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2017
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
// ************************************************************************** //

#ifndef DATASET_H
#define DATASET_H

#include "typ/angles.h"
#include "typ/curve.h"
#include "typ/image.h"
#include "typ/variant.h"

class Session;
class Metadata;
class OneDataset;
class Dataset;
class Datasets;

typedef QSharedPointer<OneDataset const> shp_OneDataset;
typedef QSharedPointer<Dataset> shp_Dataset;

//! Metadata + Image, for calculation always accessed through its owning Dataset

class OneDataset final {
    friend class OneDatasets;
    friend class Dataset;

public:
    OneDataset(Metadata const&, typ::inten_arr const&);
    OneDataset(Metadata const&, typ::size2d const&, inten_vec const&);
    OneDataset(OneDataset const&);

    QSharedPointer<Metadata const> metadata() const;

    typ::deg midTth() const;

    qreal monitorCount() const;
    qreal deltaMonitorCount() const;
    qreal deltaTime() const;

    typ::deg omg() const;
    typ::deg phi() const;
    typ::deg chi() const;

    typ::Range rgeGma(Session const&) const;
    typ::Range rgeGmaFull(Session const&) const;
    typ::Range rgeTth(Session const&) const;

    typ::Range rgeInten() const;

    typ::shp_Image image() const { return image_; }
    typ::size2d imageSize() const;

    void collectIntens(
        Session const&, typ::Image const* intensCorr, inten_vec&, uint_vec&, typ::Range const&,
        typ::deg minTth, typ::deg deltaTth) const;

private:
    QSharedPointer<Metadata const> md_;
    typ::shp_Image image_;
};


//! Collection of (OneDataset)s

class OneDatasets : public typ::vec<shp_OneDataset> {
private:
public:
    typ::size2d imageSize() const;
    typ::shp_Image foldedImage() const;
};

//! One or more OneDataset(s)

class Dataset final : public OneDatasets {
private:
    using super = OneDatasets;
    friend class Datasets;

public:
    Dataset();

    QSharedPointer<Metadata const> metadata() const;
    Datasets const& datasets() const;

    typ::deg omg() const;
    typ::deg phi() const;
    typ::deg chi() const;

    typ::Range rgeGma(Session const&) const;
    typ::Range rgeGmaFull(Session const&) const;
    typ::Range rgeTth(Session const&) const;

    typ::Range rgeInten() const;

    qreal avgMonitorCount() const;
    qreal avgDeltaMonitorCount() const;
    qreal avgDeltaTime() const;

    inten_vec collectIntens(Session const&, typ::Image const* intensCorr, typ::Range const&) const;
    void calculateAlphaBeta(typ::deg tth, typ::deg gma, typ::deg& alpha, typ::deg& beta) const;

private:
    // all dataset(s) must have the same image size
    typ::size2d imageSize() const;

    Datasets* datasets_; // here it belongs (or can be nullptr)
    QSharedPointer<Metadata const> md_; // on demand, compute once
};

//! Collection of (Dataset)s

class Datasets final : public typ::vec<shp_Dataset> {
private:
    using super = typ::vec<shp_Dataset>;
public:
    Datasets();

    void appendHere(shp_Dataset);

    typ::size2d imageSize() const;

    qreal avgMonitorCount() const;
    qreal avgDeltaMonitorCount() const;
    qreal avgDeltaTime() const;

    typ::Range const& rgeGma(Session const&) const;
    typ::Range const& rgeFixedInten(Session const&, bool trans, bool cut) const;

    typ::Curve avgCurve(Session const&) const;

    void invalidateAvgMutables() const;

private:
    shp_Dataset combineAll() const;
    qreal calcAvgMutable(qreal (Dataset::*avgMth)() const) const;

    // computed on demand (NaNs or emptiness indicate yet unknown values)
    mutable qreal avgMonitorCount_, avgDeltaMonitorCount_, avgDeltaTime_;
    mutable typ::Range rgeFixedInten_;
    mutable typ::Range rgeGma_;
    mutable typ::Curve avgCurve_;
};

Q_DECLARE_METATYPE(shp_Dataset)

#endif // DATASET_H
