/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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

#include "fs/pln/flightplanconstants.h"

#include <QHash>

namespace atools {
namespace fs {
namespace pln {

void copyProperty(QHash<QString, QString>& to, const QHash<QString, QString>& from, const QString& key)
{
  to.remove(key);
  if(from.contains(key))
    to.insert(key, from.value(key));
}

void copySidProcedureProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from)
{
  copyProperty(to, from, SID);
  copyProperty(to, from, SID_RW);
  copyProperty(to, from, SID_TYPE);
  copyProperty(to, from, SID_TRANS);
  copyProperty(to, from, SID_TRANS_WP);
  copyProperty(to, from, DEPARTURE_CUSTOM_DISTANCE);
}

void copyArrivalProcedureProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from)
{
  copyProperty(to, from, TRANSITION);
  copyProperty(to, from, TRANSITION_TYPE);
  copyProperty(to, from, APPROACH);
  copyProperty(to, from, APPROACH_ARINC);
  copyProperty(to, from, APPROACH_TYPE);
  copyProperty(to, from, APPROACH_RW);
  copyProperty(to, from, APPROACH_SUFFIX);
  copyProperty(to, from, APPROACH_CUSTOM_DISTANCE);
  copyProperty(to, from, APPROACH_CUSTOM_ALTITUDE);
  copyProperty(to, from, APPROACH_CUSTOM_OFFSET);
}

void copyStarProcedureProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from)
{
  copyProperty(to, from, STAR);
  copyProperty(to, from, STAR_RW);
  copyProperty(to, from, STAR_TRANS);
  copyProperty(to, from, STAR_TRANS_WP);
}

} // namespace pln
} // namespace fs
} // namespace atools
