/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_BGL_NAV_ILS_H
#define ATOOLS_BGL_NAV_ILS_H

#include "fs/bgl/nav/navbase.h"

namespace atools {
namespace fs {
namespace bgl {

class Localizer;
class Glideslope;
class Dme;

/*
 * ILS station that is a top level record. It assigned to the airport by using the airport ICAO ident
 * (from NavBase::getAirportIdent) and the runway name (Localizer::getRunwayName)
 */
class Ils :
  public atools::fs::bgl::NavBase
{
public:
  /*
   * Read ILS and all optional subrecords (DME, GS and Localizer)
   */
  Ils(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *stream);
  virtual ~Ils() override;

  Ils(const Ils& other) = delete;
  Ils& operator=(const Ils& other) = delete;

  /*
   * @return DME if available - otherwise null
   */
  const atools::fs::bgl::Dme *getDme() const
  {
    return dme;
  }

  /*
   * @return Glideslope if available - otherwise null
   */
  const atools::fs::bgl::Glideslope *getGlideslope() const
  {
    return glideslope;
  }

  /*
   * @return true if this ILS also has a backcourse approach
   */
  bool hasBackcourse() const
  {
    return backcourse;
  }

  QString getType() const;

  /*
   * @return Localizer if available - otherwise null
   */
  const atools::fs::bgl::Localizer *getLocalizer() const
  {
    return localizer;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Ils& record);

  bool backcourse;

  atools::fs::bgl::Localizer *localizer = nullptr;
  atools::fs::bgl::Glideslope *glideslope = nullptr;
  atools::fs::bgl::Dme *dme = nullptr;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_NAV_ILS_H
