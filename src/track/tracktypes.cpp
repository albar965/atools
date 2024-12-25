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

#include "track/tracktypes.h"

#include <QDebug>

namespace atools {
namespace track {


QDebug operator<<(QDebug out, const Track& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << "Track["
                          << record.name
                          << ", " << typeToString(record.type)
                          << ", fully valid " << record.isFullyValid()
                          << ", valid " << record.isValid()
                          << ", " << record.route;

  if(!record.eastLevels.isEmpty())
    out.nospace().noquote() << ", east levels " << record.eastLevels;

  if(!record.westLevels.isEmpty())
    out.nospace().noquote() << ", west levels " << record.westLevels;

  out.nospace().noquote() << ", " << record.validFrom.toString("yyyy-MM-dd HH:mm")
                          << " to " << record.validTo.toString("yyyy-MM-dd HH:mm")
                          << "]";
  return out;
}

QString typeToString(TrackType type)
{
  switch(type)
  {
    case atools::track::UNKNOWN:
      return QObject::tr("Unknown");

    case atools::track::NAT:
      return QObject::tr("NAT");

    case atools::track::PACOTS:
      return QObject::tr("PACOTS");
  }
  return QString();
}

} // namespace track
} // namespace atools
