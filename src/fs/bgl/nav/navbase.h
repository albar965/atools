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

#ifndef ATOOLS_BGL_NAV_NAVBASE_H
#define ATOOLS_BGL_NAV_NAVBASE_H

#include "fs/bgl/bglposition.h"
#include "io/binarystream.h"
#include "fs/bgl/record.h"

#include <QString>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

/*
 * Base class for all radio navaids (ILS, VOR and NDB)
 */
class NavBase :
  public atools::fs::bgl::Record
{
public:
  NavBase(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);
  virtual ~NavBase();

  /*
   * @return optional airport ICAO ident
   */
  const QString& getAirportIdent() const
  {
    return airportIdent;
  }

  /*
   * @return Frequency in MHz * 1000 for VOR and ILS and kHz * 100 for NDB
   */
  int getFrequency() const
  {
    return frequency;
  }

  /*
   * @return ICAO ident of the navaid
   */
  const QString& getIdent() const
  {
    return ident;
  }

  /*
   * @return Magnetic variance for the navaid. < 0 for West and > 0 for East
   */
  float getMagVar() const
  {
    return magVar;
  }

  /*
   * @return Name of the navaid if available
   */
  const QString& getName() const
  {
    return name;
  }

  /*
   * @return Position of the navaid
   */
  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  /*
   * @return Range of the navaid in meter
   */
  float getRange() const
  {
    return range;
  }

  /*
   * @return Two letter ICAO region ident for the navaid if available
   */
  const QString& getRegion() const
  {
    return region;
  }

  virtual bool isValid() const override;
  virtual QString getObjectName() const override;

protected:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::NavBase& record);

  atools::fs::bgl::BglPosition position;
  int frequency;
  float range, magVar;
  QString ident, region, airportIdent, name;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_NAV_NAVBASE_H
