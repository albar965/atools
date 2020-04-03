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

#ifndef ATOOLS_TRACKTYPES_H
#define ATOOLS_TRACKTYPES_H

#include <QDateTime>
#include <QVector>

namespace atools {
namespace track {

/* Has to match database defintions in create_track_schema.sql */
enum TrackType : char
{
  UNKNOWN = '\0',
  NAT = 'N',
  PACOTS = 'P',
  AUSOTS = 'A'
};

static const QVector<TrackType> ALL_TRACK_TYPES = {NAT, PACOTS, AUSOTS};

enum TrackDirection : char
{
  NONE = '\0',
  EAST = 'E',
  WEST = 'W',
  BOTH = 'B'
};

QString typeToString(atools::track::TrackType type);

struct Track
{
  /* Name, number or character. */
  QString name;

  TrackType type = UNKNOWN;
  TrackDirection direction = NONE;

  /* Waypoint names, coordinates and/or airway idents. */
  QStringList route;

  /* Eastern or western flight levels. Only for NAT. */
  QVector<short> eastLevels, westLevels;

  /* Validity period. All dates in UTC. */
  QDateTime validFrom, validTo;

  /* Quick check for validity. false if default constructed. */
  bool isValid() const
  {
    return !route.isEmpty();
  }

  /* Checks waypoints, type, levels and valid times. */
  bool isFullyValid() const
  {
    return type != UNKNOWN && route.size() >= 2 &&
           !name.isEmpty() && validTo.isValid() && validFrom.isValid() &&
           (type != NAT || eastLevels.size() + westLevels.size() > 0);
  }

  QString typeString() const
  {
    return typeToString(type);
  }

};

typedef QVector<atools::track::Track> TrackVectorType;

QDebug operator<<(QDebug out, const atools::track::Track& record);

} // namespace track
} // namespace atools

Q_DECLARE_TYPEINFO(atools::track::Track, Q_MOVABLE_TYPE);

#endif // ATOOLS_TRACKTYPES_H
