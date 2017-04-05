/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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
#include "geo/pos.h"

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

/*
 * A primary or secondary runway end. This class is filled in the Runway class
 * and has no correspoinding BGL record.
 */
class RunwayEnd
{
public:
  RunwayEnd();
  virtual ~RunwayEnd();

  /*
   * @return Full runway end name like "12C" or "24"
   */
  QString getName() const;

  const RunwayApproachLights& getApproachLights() const
  {
    return approachLights;
  }

  /*
   * @return Length of blast pad in meter. This is not a part of the runway length.
   */
  int getBlastPad() const
  {
    return blastPad;
  }

  const RunwayVasi& getLeftVasi() const
  {
    return leftVasi;
  }

  /*
   * @return Length of a displaced threshold meter. This is a part of the runway length and
   * reduces effective landing distance.
   */
  int getOffsetThreshold() const
  {
    return offsetThreshold;
  }

  /*
   * @return Length of overrun area in in meter. This is not a part of the runway length.
   */
  int getOverrun() const
  {
    return overrun;
  }

  const RunwayVasi& getRightVasi() const
  {
    return rightVasi;
  }

  /*
   * @return true if runway end has a yellow X and is closed for landing and takeoff
   */
  bool hasClosedMarkings() const
  {
    return closedMarkings;
  }

  /*
   * @return true if the runway end is used for landing (affects FS ATC and traffic)
   */
  bool isLanding() const
  {
    return landing;
  }

  /*
   * @return Pattern direction. Affects only traffic
   */
  atools::fs::bgl::rw::Pattern getPattern() const
  {
    return pattern;
  }

  /*
   * @return true if the is a STOL marking on the runway end
   */
  bool hasStolMarkings() const
  {
    return stolMarkings;
  }

  /*
   * @return true if the runway end is used for takeoff (affects FS ATC and traffic)
   */
  bool isTakeoff() const
  {
    return takeoff;
  }

  static QString patternToStr(atools::fs::bgl::rw::Pattern pattern);

  bool isPrimaryEnd() const
  {
    return primaryEnd;
  }

  const atools::geo::Pos& getPosition() const
  {
    return pos;
  }

  float getHeading() const
  {
    return heading;
  }

  const QString& getIlsIdent() const
  {
    return ilsIdent;
  }

private:
  friend class Runway;
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::RunwayEnd& record);

  QString designatorStr(int designator) const;

  int number, designator;
  int offsetThreshold, blastPad, overrun;
  QString ilsIdent;

  atools::fs::bgl::RunwayVasi leftVasi, rightVasi;

  atools::fs::bgl::RunwayApproachLights approachLights;

  float heading;
  bool primaryEnd = true;
  bool closedMarkings, stolMarkings, takeoff, landing;
  atools::fs::bgl::rw::Pattern pattern;
  atools::geo::Pos pos;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AP_RW_RUNWAYEND_H
