//  ***********************************************************************************************
//
//  Steca: stress and texture calculator
//
//! @file      gui/panels/subframe_clusters.cpp
//! @brief     Implements class SubframeClusters, with local model and view
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
//  ***********************************************************************************************

#include "gui/panels/subframe_clusters.h"
#include "core/session.h"
#include "core/def/idiomatic_for.h"
#include "qcr/widgets/tables.h"
#include "qcr/base/debug.h"

//  ***********************************************************************************************
//! @class ActiveClustersModel (local scope)

//! The model for ActiveClustersView.

class ActiveClustersModel : public CheckTableModel { // < QAbstractTableModel < QAbstractItemModel
public:
    ActiveClustersModel() : CheckTableModel("measurement") {}
    void activateCluster(bool, int, bool);
    int rowCount() const final { return gSession->dataset().countClusters(); }
    int highlighted() const final { return gSession->dataset().highlight().clusterIndex(); }
    void setHighlight(int row) final { gSession->dataset().highlight().setCluster(row); }
    bool activated(int row) const { return gSession->dataset().clusterAt(row).isActivated(); }
    void setActivated(int row, bool on) { gSession->dataset().activateCluster(row, on); }

    enum { COL_CHECK=1, COL_NUMBER, COL_ATTRS };

private:
    QVariant data(const QModelIndex&, int) const final;
    QVariant headerData(int, Qt::Orientation, int) const final;
    int columnCount() const final { return COL_ATTRS + gSession->metaSelectedCount(); }
};

QVariant ActiveClustersModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= rowCount())
        return {};
    const Cluster& cluster = gSession->dataset().clusterAt(row);
    int col = index.column();
    switch (role) {
    case Qt::DisplayRole: {
        if (row==0 && col==1)
            qDebug() << "subframe Cluster: call to data(), meta#=" << gSession->metaSelectedCount();
        if (col==COL_NUMBER) {
            QString ret = QString::number(cluster.totalOffset()+1);
            if (cluster.count()>1)
                ret += "-" + QString::number(cluster.totalOffset()+cluster.count());
            return ret;
        } else if (col>=COL_ATTRS && col < COL_ATTRS+gSession->metaSelectedCount()) {
            return cluster.avgMetadata().attributeStrValue(gSession->metaSelectedAt(col-COL_ATTRS));
        } else
            return {};
    }
    case Qt::ToolTipRole: {
        QString ret;
        if (cluster.count()>1) {
            ret = QString("Measurements %1..%2 are numbers %3..%4 in file %5")
                .arg(cluster.totalOffset()+1)
                .arg(cluster.totalOffset()+cluster.count())
                .arg(cluster.offset()+1)
                .arg(cluster.offset()+cluster.count())
                .arg(cluster.file().name());
        } else {
            ret = QString("Measurement %1 is number %2 in file %3")
                .arg(cluster.totalOffset()+1)
                .arg(cluster.offset()+1)
                .arg(cluster.file().name());
        }
        ret += ".";
        if (cluster.isIncomplete())
            ret += QString("\nThis cluster has only %1 elements, while the binning factor is %2.")
                .arg(cluster.count())
                .arg(gSession->dataset().binning.val());
        return ret;
    }
    case Qt::ForegroundRole: {
        if (col==COL_NUMBER && cluster.count()>1 &&
            (cluster.isIncomplete()))
            return QColor(Qt::red);
        return QColor(Qt::black);
    }
    case Qt::BackgroundRole: {
        if (row==highlighted())
            return QColor(Qt::cyan);
        return QColor(Qt::white);
    }
    case Qt::CheckStateRole: {
        if (col==COL_CHECK)
            return activated(row) ? Qt::Checked : Qt::Unchecked;
        return {};
    }
    default:
        return {};
    }
}

QVariant ActiveClustersModel::headerData(int col, Qt::Orientation ori, int role) const
{
    if (ori!=Qt::Horizontal)
        return {};
    if (role != Qt::DisplayRole)
        return {};
    if (col==COL_NUMBER)
        return "#";
    else if (col>=COL_ATTRS && col < COL_ATTRS+gSession->metaSelectedCount())
        return Metadata::attributeTag(gSession->metaSelectedAt(col-COL_ATTRS), false);
    return {};
}


//  ***********************************************************************************************
//! @class ActiveClustersView (local scope)

//! Main item in SubframeMeasurement: View and control of measurements list.

class ActiveClustersView : public CheckTableView { // < QTreeView < QAbstractItemView
public:
    ActiveClustersView();
private:
    void currentChanged(const QModelIndex& current, const QModelIndex&) override final {
        gotoCurrent(current); }
    void refresh();
    int sizeHintForColumn(int) const override final;
    ActiveClustersModel* model() { return static_cast<ActiveClustersModel*>(model_); }
    void onData() override;
};

ActiveClustersView::ActiveClustersView()
    : CheckTableView(new ActiveClustersModel())
{
    setSelectionMode(QAbstractItemView::NoSelection);
    onData();
}

void ActiveClustersView::onData()
{
    setHeaderHidden(gSession->metaSelectedCount()==0);
    model_->refreshModel();
    updateScroll();
}

int ActiveClustersView::sizeHintForColumn(int col) const
{
    switch (col) {
    case ActiveClustersModel::COL_CHECK: {
        return 2*mWidth();
    } default:
        return 3*mWidth();
    }
}


//  ***********************************************************************************************
//! @class SubframeClusters

SubframeClusters::SubframeClusters()
    : QcrDockWidget("measurements")
{
    setFeatures(DockWidgetMovable);
    setWindowTitle("Measurements");
    setWidget(new ActiveClustersView()); // list of Cluster|s
}
