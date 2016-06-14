/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef ATOOLS_BGL_AP_RW_RUNWAYEND_H
#define ATOOLS_BGL_AP_RW_RUNWAYEND_H

#include "fs/bgl/ap/rw/runwayapplights.h"
#include "fs/bgl/ap/rw/runwayvasi.h"

#include <QString>

namespace atools {
namespace fs {
namespace bgl {

namespace rw {

enum Pattern
{
  LEFT = 0, RIGHT = 1
};

} // namespace rw

class RunwayEnd
{
public:
  RunwayEnd();
  virtual ~RunwayEnd();

  QString getName() const;

  const RunwayAppLights& getApproachLights() const
  {
    return approachLights;
  }

  int getBlastPad() const
  {
    return blastPad;
  }

  const RunwayVasi& getLeftVasi() const
  {
    return leftVasi;
  }

  int getOffsetThreshold() const
  {
    return offsetThreshold;
  }

  int getOverrun() const
  {
    return overrun;
  }

  const RunwayVasi& getRightVasi() const
  {
    return rightVasi;
  }

  bool hasClosedMarkings() const
  {
    return closedMarkings;
  }

  bool isLanding() const
  {
    return landing;
  }

  atools::fs::bgl::rw::Pattern getPattern() const
  {
    return pattern;
  }

  bool hasStolMarkings() const
  {
    return stolMarkings;
  }

  bool isTakeoff() const
  {
    return takeoff;
  }

  static QString patternToStr(atools::fs::bgl::rw::Pattern pattern);

private:
  friend class Runway;
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::RunwayEnd& record);

  QString designatorStr(int designator) const;

  int number, designator;
  int offsetThreshold, blastPad, overrun;

  atools::fs::bgl::RunwayVasi leftVasi, rightVasi;

  atools::fs::bgl::RunwayAppLights approachLights;

  bool closedMarkings, stolMarkings, takeoff, landing;
  atools::fs::bgl::rw::Pattern pattern;

};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AP_RW_RUNWAYEND_H
