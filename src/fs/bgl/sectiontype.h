/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_BGL_SECTIONTYPE_H
#define ATOOLS_BGL_SECTIONTYPE_H

#include <QString>

namespace atools {
namespace fs {
namespace bgl {

namespace section {

/* See SUPPORTED_SECTION_TYPES in datawriter.cpp to see what is actually read/supported */
enum SectionType
{
  NONE = 0x0,
  COPYRIGHT = 0x01,
  GUID = 0x02,
  AIRPORT = 0x03,
  AIRPORT_ALT = 0x3C, // Alternative record ID mentioned in some manuals
  ILS_VOR = 0x13,
  NDB = 0x17,
  MARKER = 0x18,
  BOUNDARY = 0x20,
  WAYPOINT = 0x22,
  GEOPOL = 0x23,
  SCENERY_OBJECT = 0x25,
  NAME_LIST = 0x27,
  VOR_ILS_ICAO_INDEX = 0x28,
  NDB_ICAO_INDEX = 0x29,
  WAYPOINT_ICAO_INDEX = 0x2A,
  MODEL_DATA = 0x2B,
  AIRPORT_SUMMARY = 0x2C,
  EXCLUSION = 0x2E,
  TIMEZONE = 0x2F,
  TERRAIN_VECTOR_DB = 0x65,
  TERRAIN_ELEVATION = 0x67,
  TERRAIN_LAND_CLASS = 0x68,
  TERRAIN_WATER_CLASS = 0x69,
  TERRAIN_REGION = 0x6A,
  POPULATION_DENSITY = 0x6C,
  AUTOGEN_ANNOTATION = 0x6D,
  TERRAIN_INDEX = 0x6E,
  TERRAIN_TEXTURE_LOOKUP = 0x6F,
  TERRAIN_SEASON_JAN = 0x78,
  TERRAIN_SEASON_FEB = 0x79,
  TERRAIN_SEASON_MAR = 0x7A,
  TERRAIN_SEASON_APR = 0x7B,
  TERRAIN_SEASON_MAY = 0x7C,
  TERRAIN_SEASON_JUN = 0x7D,
  TERRAIN_SEASON_JUL = 0x7E,
  TERRAIN_SEASON_AUG = 0x7F,
  TERRAIN_SEASON_SEP = 0x80,
  TERRAIN_SEASON_OCT = 0x81,
  TERRAIN_SEASON_NOV = 0x82,
  TERRAIN_SEASON_DEC = 0x83,
  TERRAIN_PHOTO_JAN = 0x8C,
  TERRAIN_PHOTO_FEB = 0x8D,
  TERRAIN_PHOTO_MAR = 0x8E,
  TERRAIN_PHOTO_APR = 0x8F,
  TERRAIN_PHOTO_MAY = 0x90,
  TERRAIN_PHOTO_JUN = 0x91,
  TERRAIN_PHOTO_JUL = 0x92,
  TERRAIN_PHOTO_AUG = 0x93,
  TERRAIN_PHOTO_SEP = 0x94,
  TERRAIN_PHOTO_OCT = 0x95,
  TERRAIN_PHOTO_NOV = 0x96,
  TERRAIN_PHOTO_DEC = 0x97,
  TERRAIN_PHOTO_NIGHT = 0x98,
  TACAN = 0xa0, /* P3D only */

  FAKE_TYPES = 0x2710,
  ICAO_RUNWAY = 0x2711

};

QString sectionTypeStr(atools::fs::bgl::section::SectionType type);

} // namespace section

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_SECTIONTYPE_H
