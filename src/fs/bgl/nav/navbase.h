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

#ifndef BGL_NAV_NAVBASE_H_
#define BGL_NAV_NAVBASE_H_

#include "fs/bgl/bglposition.h"
#include "fs/bgl/record.h"
#include <QString>
#include <io/binarystream.h>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

class NavBase :
  public atools::fs::bgl::Record
{
public:
  NavBase(const atools::fs::BglReaderOptions *options, atools::io::BinaryStream *bs);
  virtual ~NavBase();

  const QString& getAirportIdent() const
  {
    return airportIdent;
  }

  int getFrequency() const
  {
    return frequency;
  }

  const QString& getIdent() const
  {
    return ident;
  }

  float getMagVar() const
  {
    return magVar;
  }

  const QString& getName() const
  {
    return name;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  float getRange() const
  {
    return range;
  }

  const QString& getRegion() const
  {
    return region;
  }

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

#endif /* BGL_NAV_NAVBASE_H_ */
