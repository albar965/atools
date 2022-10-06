/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include "geo/point3d.h"

#include <QDebug>

namespace atools {
namespace geo {

QDebug operator<<(QDebug out, const Point3D& pt)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using Qt::fixed;
#endif

  QDebugStateSaver saver(out);
  out.nospace().noquote() << fixed << qSetRealNumberPrecision(1)
                          << "Point3D(x " << pt.x << ", y " << pt.y << ", z " << pt.z
                          << ", valid " << pt.isValid() << ")";
  return out;
}

} // namespace geo
} // namespace atools
