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

#ifndef BGL_AP_TRANSITION_H_
#define BGL_AP_TRANSITION_H_

#include "fs/bgl/record.h"

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
  APPR_TRANS_FULL = 1,
  APPR_TRANS_DME = 2
};

enum TransitionFixType
{
  APPR_VOR = 2,
  APPR_NDB = 3,
  APPR_TERMINAL_NDB = 4,
  APPR_WAYPOINT = 5,
  APPR_TERMINAL_WAYPOINT = 6
};

} // namespace ap

class Transition :
  public atools::fs::bgl::Record
{
public:
  Transition(atools::io::BinaryStream *bs);
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

  int getNumLegs() const
  {
    return numLegs;
  }

  const QString& getTransFixIdent() const
  {
    return transFixIdent;
  }

  atools::fs::bgl::ap::TransitionFixType getTransFixType() const
  {
    return transFixType;
  }

  atools::fs::bgl::ap::TransitionType getType() const
  {
    return type;
  }

  static QString transitionTypeToStr(atools::fs::bgl::ap::TransitionType type);
  static QString transitionFixTypeToStr(atools::fs::bgl::ap::TransitionFixType type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Transition& record);

  atools::fs::bgl::ap::TransitionType type;
  int numLegs;
  atools::fs::bgl::ap::TransitionFixType transFixType;
  QString transFixIdent, fixRegion, fixAirportIdent;
  float altitude;
  QString dmeIdent, dmeRegion, dmeAirportIdent;
  int dmeRadial;
  float dmeDist;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AP_TRANSITION_H_ */
