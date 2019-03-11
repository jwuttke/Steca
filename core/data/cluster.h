//  ***********************************************************************************************
//
//  Steca: stress and texture calculator
//
//! @file      core/data/cluster.h
//! @brief     Defines classes Sequence, Cluster
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
//  ***********************************************************************************************

#ifndef CLUSTER_H
#define CLUSTER_H

#include "core/data/dfgram.h"
#include "core/raw/measurement.h"
#include "core/typ/lazy_data.h"

//! A group of one or more `Measurement`s.

//! Base class of Cluster, and also used to hold _all_ loaded Measurements.
//!
//! `Measurement`s are always owned by `Datafile`s; here they are accessed through const pointers.

class Sequence {
public:
    Sequence() = delete;
    Sequence(const Sequence&) = delete;
    Sequence(const std::vector<const Measurement*>& measurements);

    const int size() const { return members_.size(); }
    const Measurement* first() const { return members_.front(); }
    const Measurement* at(int i) const { return members_.at(i); }
    const std::vector<const Measurement*>& members() const { return members_; }

    deg omg() const;
    deg phi() const;
    deg chi() const;

    Range rangeGma() const;
    Range rangeGmaFull() const;
    Range rangeTth() const;
    Range rangeInten() const;
    double normFactor() const;

    const Metadata& avgMetadata() const { return metadata_; }

    size2d imageSize() const;

private:
    double avgMonitorCount() const;
    double avgDeltaMonitorCount() const;
    double avgTime() const;
    double avgDeltaTime() const;

    const std::vector<const Measurement*> members_; //!< ptr to Dataset:vec<Datafile>:vec<M'ments>
    const Metadata metadata_; //!< averaged Metadata

    Metadata computeAvgMetadata() const;
};


//! A group of one or more `Measurement`s, with associated information.

class Cluster : public Sequence {
public:
    Cluster() = delete;
    Cluster(const std::vector<const Measurement*>& measurements,
            const class Datafile& file, const int index, const int offset);
    Cluster(const Cluster&) = delete;

    void setSelected(bool on) { selected_ = on; }

    const class Datafile& file() const { return file_; }
    int index() const { return index_; }
    int offset() const { return offset_; }
    int totalOffset() const;
    bool isIncomplete() const;
    bool isActive() const;
    bool isSelected() const { return selected_; }
    Qt::CheckState state() const;

    mutable lazy_data::VectorCache<Dfgram,const Cluster*> dfgrams; //! One Dfgram per gamma section
    const Dfgram& currentDfgram() const;

private:
    const class Datafile& file_;
    const int index_; //!< index in total list of `Cluster`s
    const int offset_; //!< index of first Measurement in file_
    bool selected_; //!< selected for use
};

#endif // CLUSTER_H
