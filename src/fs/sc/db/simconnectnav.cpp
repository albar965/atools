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

#include "fs/sc/db/simconnectnav.h"
#include "fs/bgl/nav/airwaysegment.h"

#include <QVariant>

namespace atools {
namespace fs {
namespace sc {
namespace db {

int Waypoint::getNumVictorAirway() const
{
  int num = 0;
  for(const RouteFacility& route:routes)
  {
    bgl::nav::AirwayType type = static_cast<bgl::nav::AirwayType>(route.type);
    if(type == bgl::nav::VICTOR || type == bgl::nav::BOTH)
      num++;
  }
  return num;
}

int Waypoint::getNumJetAirway() const
{
  int num = 0;
  for(const RouteFacility& route:routes)
  {
    bgl::nav::AirwayType type = static_cast<bgl::nav::AirwayType>(route.type);
    if(type == bgl::nav::JET || type == bgl::nav::BOTH)
      num++;
  }
  return num;
}

QString waypointTypeToRouteDb(char type)
{
  switch(type)
  {
    case 'V':
    case 'N':
      return QString(QChar(type));

    case 'W':
      return "O";
  }

  qWarning() << Q_FUNC_INFO << "Invalid airway waypoint type " << type;
  return "\0";
}

QVariant vorTypeToDb(VorType type, bool isNav, bool isTacan)
{
  // VORTAC FFM ED VOR DME 1 NAV 1 TACAN 1
  // VOR    MTR ED VOR DME 0 NAV 1 TACAN 0
  // VORDME ZWN ED VOR DME 1 NAV 1 TACAN 0
  // DME    XXX ED VOR DME 1 NAV 0 TACAN 0
  // TACAN  GIX ED VOR DME 1 NAV 0 TACAN 1

  if(isTacan)
  {
    if(isNav)
    {
      // VORTAC
      switch(type)
      {
        case TERMINAL:
          return "VTT";

        case LOW_ALTITUDE:
        case LOW_ALT:
          return "VTL";

        case HIGH_ALTITUDE:
        case HIGH_ALT:
          return "VTH";

        case VOR_UNKNOWN:
          return "VT";

        case VOT:
        case ILS:
          break;
      }
    }
    else
      // TACAN
      return "TC";
  }
  else
  {
    // VOR, VORDME or DME
    switch(type)
    {
      case TERMINAL:
        return "T";

      case LOW_ALTITUDE:
      case LOW_ALT:
        return "L";

      case HIGH_ALTITUDE:
      case HIGH_ALT:
        return "H";

      case VOT:
        return "V";

      case ILS:
        return "I";

      case VOR_UNKNOWN:
        break;
    }
  }

  qWarning() << Q_FUNC_INFO << "Invalid VOR type " << type;
  return QVariant(QVariant::String);
}

bool lsTypeValid(LsCategory type)
{
  return type > NONE && type <= LAST;
}

QVariant lsTypeToDb(LsCategory type)
{
  // type varchar(1),             -- null, unknown
  // -- ILS Localizer only, no glideslope   0
  // -- ILS Localizer/MLS/GLS Unknown cat   U
  // -- ILS Localizer/MLS/GLS Cat I         1
  // -- ILS Localizer/MLS/GLS Cat II        2
  // -- ILS Localizer/MLS/GLS Cat III       3
  // -- IGS Facility                        I
  // -- LDA Facility with glideslope        L
  // -- LDA Facility no glideslope          A
  // -- SDF Facility with glideslope        S
  // -- SDF Facility no glideslope          F
  // -- GLS ground station:                 G
  // -- GLS / GBAS threshold point:         T
  switch(type)
  {
    case LOCALIZER:
      return "0";

    case CAT1:
      return "1";

    case CAT2:
      return "2";

    case CAT3:
      return "3";

    case IGS:
      return "I";

    case LDA_WITH_GS:
      return "L";
    case LDA_NO_GS:
      return "A";

    case SDF_WITH_GS:
      return "S";

    case SDF_NO_GS:
      return "F";

    case NONE:
      break;
  }
  qWarning() << Q_FUNC_INFO << "Invalid ILS type " << type;
  return QVariant(QVariant::String);
}

} // namespace db
} // namespace sc
} // namespace fs
} // namespace atools
