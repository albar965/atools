/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#include "fs/bgl/sectiontype.h"

namespace atools {
namespace fs {
namespace bgl {
namespace section {

QString sectionTypeStr(atools::fs::bgl::section::SectionType type)
{
  switch(type)
  {
    case section::NONE:
      return "NONE";

    case section::COPYRIGHT:
      return "COPYRIGHT";

    case section::GUID:
      return "GUID";

    case section::AIRPORT:
      return "AIRPORT";

    case section::AIRPORT_ALT:
      return "AIRPORT_ALT";

    case section::ILS_VOR:
      return "ILS_VOR";

    case section::NDB:
      return "NDB";

    case section::MARKER:
      return "MARKER";

    case section::BOUNDARY:
      return "BOUNDARY";

    case section::WAYPOINT:
      return "WAYPOINT";

    case section::GEOPOL:
      return "GEOPOL";

    case section::SCENERY_OBJECT:
      return "SCENERY_OBJECT";

    case section::NAME_LIST:
      return "NAME_LIST";

    case section::VOR_ILS_ICAO_INDEX:
      return "VOR_ILS_ICAO_INDEX";

    case section::NDB_ICAO_INDEX:
      return "NDB_ICAO_INDEX";

    case section::WAYPOINT_ICAO_INDEX:
      return "WAYPOINT_ICAO_INDEX";

    case section::MODEL_DATA:
      return "MODEL_DATA";

    case section::AIRPORT_SUMMARY:
      return "AIRPORT_SUMMARY";

    case section::EXCLUSION:
      return "EXCLUSION";

    case section::TIMEZONE:
      return "TIMEZONE";

    case section::TERRAIN_VECTOR_DB:
      return "TERRAIN_VECTOR_DB";

    case section::TERRAIN_ELEVATION:
      return "TERRAIN_ELEVATION";

    case section::TERRAIN_LAND_CLASS:
      return "TERRAIN_LAND_CLASS";

    case section::TERRAIN_WATER_CLASS:
      return "TERRAIN_WATER_CLASS";

    case section::TERRAIN_REGION:
      return "TERRAIN_REGION";

    case section::POPULATION_DENSITY:
      return "POPULATION_DENSITY";

    case section::AUTOGEN_ANNOTATION:
      return "AUTOGEN_ANNOTATION";

    case section::TERRAIN_INDEX:
      return "TERRAIN_INDEX";

    case section::TERRAIN_TEXTURE_LOOKUP:
      return "TERRAIN_TEXTURE_LOOKUP";

    case section::TERRAIN_SEASON_JAN:
      return "TERRAIN_SEASON_JAN";

    case section::TERRAIN_SEASON_FEB:
      return "TERRAIN_SEASON_FEB";

    case section::TERRAIN_SEASON_MAR:
      return "TERRAIN_SEASON_MAR";

    case section::TERRAIN_SEASON_APR:
      return "TERRAIN_SEASON_APR";

    case section::TERRAIN_SEASON_MAY:
      return "TERRAIN_SEASON_MAY";

    case section::TERRAIN_SEASON_JUN:
      return "TERRAIN_SEASON_JUN";

    case section::TERRAIN_SEASON_JUL:
      return "TERRAIN_SEASON_JUL";

    case section::TERRAIN_SEASON_AUG:
      return "TERRAIN_SEASON_AUG";

    case section::TERRAIN_SEASON_SEP:
      return "TERRAIN_SEASON_SEP";

    case section::TERRAIN_SEASON_OCT:
      return "TERRAIN_SEASON_OCT";

    case section::TERRAIN_SEASON_NOV:
      return "TERRAIN_SEASON_NOV";

    case section::TERRAIN_SEASON_DEC:
      return "TERRAIN_SEASON_DEC";

    case section::TERRAIN_PHOTO_JAN:
      return "TERRAIN_PHOTO_JAN";

    case section::TERRAIN_PHOTO_FEB:
      return "TERRAIN_PHOTO_FEB";

    case section::TERRAIN_PHOTO_MAR:
      return "TERRAIN_PHOTO_MAR";

    case section::TERRAIN_PHOTO_APR:
      return "TERRAIN_PHOTO_APR";

    case section::TERRAIN_PHOTO_MAY:
      return "TERRAIN_PHOTO_MAY";

    case section::TERRAIN_PHOTO_JUN:
      return "TERRAIN_PHOTO_JUN";

    case section::TERRAIN_PHOTO_JUL:
      return "TERRAIN_PHOTO_JUL";

    case section::TERRAIN_PHOTO_AUG:
      return "TERRAIN_PHOTO_AUG";

    case section::TERRAIN_PHOTO_SEP:
      return "TERRAIN_PHOTO_SEP";

    case section::TERRAIN_PHOTO_OCT:
      return "TERRAIN_PHOTO_OCT";

    case section::TERRAIN_PHOTO_NOV:
      return "TERRAIN_PHOTO_NOV";

    case section::TERRAIN_PHOTO_DEC:
      return "TERRAIN_PHOTO_DEC";

    case section::TERRAIN_PHOTO_NIGHT:
      return "TERRAIN_PHOTO_NIGHT";

    case section::TACAN:
      return "TACAN";

    case section::FAKE_TYPES:
      return "FAKE_TYPES";

    case section::ICAO_RUNWAY:
      return "ICAO_RUNWAY";
  }
  return QString();
}

} // namespace section
} // namespace bgl
} // namespace fs
} // namespace atools
