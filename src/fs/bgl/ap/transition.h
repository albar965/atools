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

#ifndef ATOOLS_BGL_AP_TRANSITION_H
#define ATOOLS_BGL_AP_TRANSITION_H

#include "fs/bgl/record.h"
#include "fs/bgl/ap/approachleg.h"

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace ap {

enum TransitionType
{
  FULL = 1,
  DME = 2
};

namespace tfix {
enum TransitionFixType
{
  VOR = 2,
  NDB = 3,
  TERMINAL_NDB = 4,
  WAYPOINT = 5,
  TERMINAL_WAYPOINT = 6
};

}

} // namespace ap

class Transition :
  public atools::fs::bgl::Record
{
public:
  Transition(const atools::fs::BglReaderOptions *options, atools::io::BinaryStream *bs);
  virtual ~Transition();

  const QString& getFixAirportIdent() const
  {
    return fixAirportIdent;
  }

  float getAltitude() const
  {
    return altitude;
  }

  const QString& getDmeAirportIdent() const
  {
    return dmeAirportIdent;
  }

  float getDmeDist() const
  {
    return dmeDist;
  }

  const QString& getDmeIdent() const
  {
    return dmeIdent;
  }

  int getDmeRadial() const
  {
    return dmeRadial;
  }

  const QString& getDmeRegion() const
  {
    return dmeRegion;
  }

  const QString& getFixRegion() const
  {
    return fixRegion;
  }

  const QString& getTransFixIdent() const
  {
    return transFixIdent;
  }

  atools::fs::bgl::ap::tfix::TransitionFixType getTransFixType() const
  {
    return transFixType;
  }

  atools::fs::bgl::ap::TransitionType getType() const
  {
    return type;
  }

  const QList<atools::fs::bgl::ApproachLeg>& getLegs() const
  {
    return legs;
  }

  static QString transitionTypeToStr(atools::fs::bgl::ap::TransitionType type);
  static QString transitionFixTypeToStr(atools::fs::bgl::ap::tfix::TransitionFixType type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Transition& record);

  atools::fs::bgl::ap::TransitionType type;
  atools::fs::bgl::ap::tfix::TransitionFixType transFixType;
  QString transFixIdent, fixRegion, fixAirportIdent;
  float altitude;
  QString dmeIdent, dmeRegion, dmeAirportIdent;
  int dmeRadial;
  float dmeDist;
  QList<atools::fs::bgl::ApproachLeg> legs;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AP_TRANSITION_H
