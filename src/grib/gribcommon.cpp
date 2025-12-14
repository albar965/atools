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

#include "gribcommon.h"

#include <QDebug>

namespace atools {
namespace grib {

} // namespace grib
} // namespace atools

QDebug operator<<(QDebug out, const atools::grib::GribDataset& type)
{
  QDebugStateSaver saver(out);

  out.noquote().nospace() << "GribDataset["
                          << type.getDatetime()
                          << ", surface " << type.getSurface()
                          << ", type " << type.getSurfaceType()
                          << ", param type " << type.getParameterType()
                          << ", alt round " << type.getAltFeetRounded()
                          << ", alt calc " << type.getAltFeetCalculated()
                          << ", size " << type.getData().size()
                          << "]";

  return out;
}
