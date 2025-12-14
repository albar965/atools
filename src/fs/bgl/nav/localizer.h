/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_BGL_NAV_LOCALIZER_H
#define ATOOLS_BGL_NAV_LOCALIZER_H

#include "fs/bgl/record.h"

namespace atools {
namespace fs {
namespace bgl {

/*
 * Localizer is a subrecord of ILS
 */
class Localizer :
  public atools::fs::bgl::Record
{
public:
  Localizer(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *stream, float magVar);
  virtual ~Localizer() override;

  /*
   * @return Get full runway name including designator
   */
  QString getRunwayName() const;

  /*
   * @return localizer heading degree true
   */
  float getHeading() const
  {
    return heading;
  }

  /*
   * @return Width of the localizer beam in degree
   */
  float getWidth() const
  {
    return width;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Localizer& record);

  int runwayNumber, runwayDesignator;
  float heading, width;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_NAV_LOCALIZER_H
