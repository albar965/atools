/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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
  copyProperty(to, from, SIDAPPR);
  copyProperty(to, from, SIDAPPRRW);
  copyProperty(to, from, SIDAPPRDISTANCE);
  copyProperty(to, from, SIDAPPRSIZE);
  copyProperty(to, from, SIDTRANS);
  copyProperty(to, from, SIDTRANSDISTANCE);
  copyProperty(to, from, SIDTRANSSIZE);
}

void copyArrivalProcedureProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from)
{
  copyProperty(to, from, TRANSITION);
  copyProperty(to, from, TRANSITIONTYPE);
  copyProperty(to, from, TRANSITIONDISTANCE);
  copyProperty(to, from, TRANSITIONSIZE);
  copyProperty(to, from, APPROACH);
  copyProperty(to, from, APPROACH_ARINC);
  copyProperty(to, from, APPROACHTYPE);
  copyProperty(to, from, APPROACHRW);
  copyProperty(to, from, APPROACHSUFFIX);
  copyProperty(to, from, APPROACHDISTANCE);
  copyProperty(to, from, APPROACHSIZE);
  copyProperty(to, from, APPROACH_CUSTOM_DISTANCE);
  copyProperty(to, from, APPROACH_CUSTOM_ALTITUDE);
}

void copyStarProcedureProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from)
{
  copyProperty(to, from, STAR);
  copyProperty(to, from, STARRW);
  copyProperty(to, from, STARDISTANCE);
  copyProperty(to, from, STARSIZE);
}

void copyAlternateProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from)
{
  copyProperty(to, from, ALTERNATES);
}

} // namespace pln
} // namespace fs
} // namespace atools
