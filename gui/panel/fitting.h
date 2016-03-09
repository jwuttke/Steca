/** \file
 */

#ifndef FITTING_H
#define FITTING_H

#include "panel.h"
#include "core_reflection.h"

namespace model {
class ReflectionViewModel;
}

namespace panel {
//------------------------------------------------------------------------------

class ReflectionView: public TreeListView {
  SUPER(ReflectionView,TreeListView)
public:
  using Model = model::ReflectionViewModel;

  ReflectionView(TheHub&);

  void addReflection();
  void removeSelected();
  bool hasReflections() const;

  void update();

protected:
  void selectionChanged(QItemSelection const&, QItemSelection const&);

private:
  TheHub &theHub;
  Model  &model;
};

//------------------------------------------------------------------------------

class Fitting: public BoxPanel {
  SUPER(Fitting,BoxPanel)
public:
  Fitting(TheHub&);

private:
  QSpinBox  *spinDegree;
  QComboBox *comboReflType;
  ReflectionView *reflectionView;
  QDoubleSpinBox *spinRangeMin, *spinRangeMax,
                 *spinPeakX, *spinPeakY, *spinFwhm;
  bool silentSpin;

  void enableReflControls(bool);
  void setReflControls(core::shp_Reflection const&);
};

//------------------------------------------------------------------------------
}
#endif
