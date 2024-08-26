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

#include "fs/xp/xpconstants.h"

#include "fs/bgl/ap/rw/runway.h"

namespace atools {
namespace fs {
namespace xp {

QString surfaceToDb(Surface value, const XpReaderContext *context)
{
  switch(value)
  {
    case UNKNOWN:
      return "UNKNOWN";

    case TRANSPARENT:
      return "TR";

    case TURF_OR_GRASS:
      return "G";

    case DRY_LAKEBED:
    case DIRT:
      return "D";

    case GRAVEL:
      return "GR";

    case WATER:
      return "W";

    case SNOW_OR_ICE:
      return "SN";

    case ASPHALT:
    case ASPHALT_L:
    case ASPHALT_L_PATCHED:
    case ASPHALT_L_PLAIN:
    case ASPHALT_L_WORN:
    case ASPHALT_PATCHED:
    case ASPHALT_PLAIN:
    case ASPHALT_WORN:
    case ASPHALT_D:
    case ASPHALT_D_PATCHED:
    case ASPHALT_D_PLAIN:
    case ASPHALT_D_WORN:
    case ASPHALT_D2:
    case ASPHALT_D2_PATCHED:
    case ASPHALT_D2_PLAIN:
    case ASPHALT_D2_WORN:
    case ASPHALT_D3:
    case ASPHALT_D3_PATCHED:
    case ASPHALT_D3_PLAIN:
    case ASPHALT_D3_WORN:
      return "A";

    case CONCRETE:
    case CONCRETE_L:
    case CONCRETE_L_DIRTY:
    case CONCRETE_L_WORN:
    case CONCRETE_DIRTY:
    case CONCRETE_WORN:
    case CONCRETE_D:
    case CONCRETE_D_DIRTY:
    case CONCRETE_D_WORN:
      return "C";
  }

#ifdef DEBUG_INFORMATION
  qWarning() << (context != nullptr ? context->messagePrefix() : QString()) << "Unknown surface value" << value;
#else
  Q_UNUSED(context)
#endif

  // Fall back to asphalt
  return "A";
}

bool isSurfaceHard(Surface value)
{
  return !isSurfaceSoft(value) && !isSurfaceWater(value);
}

bool isSurfaceSoft(Surface value)
{
  return value == TURF_OR_GRASS || value == DRY_LAKEBED || value == DIRT || value == GRAVEL || value == SNOW_OR_ICE;
}

bool isSurfaceWater(Surface value)
{
  return value == WATER;
}

int markingToDb(Marking value, const XpReaderContext *context)
{
  // EDGES = 1 << 0,
  // THRESHOLD = 1 << 1,
  // FIXED_DISTANCE = 1 << 2,
  // TOUCHDOWN = 1 << 3,
  // DASHES = 1 << 4,
  // IDENT = 1 << 5,
  // PRECISION = 1 << 6,
  // EDGE_PAVEMENT = 1 << 7,
  // SINGLE_END = 1 << 8,
  // PRIMARY_CLOSED = 1 << 9,
  // SECONDARY_CLOSED = 1 << 10,
  // PRIMARY_STOL = 1 << 11,
  // SECONDARY_STOL = 1 << 12,
  // ALTERNATE_THRESHOLD = 1 << 13,
  // ALTERNATE_FIXEDDISTANCE = 1 << 14,
  // ALTERNATE_TOUCHDOWN = 1 << 15,
  // ALTERNATE_PRECISION = 1 << 21,
  // LEADING_ZERO_IDENT = 1 << 22,
  // NO_THRESHOLD_END_ARROWS = 1 << 23

  using namespace atools::fs::bgl::rw;

  switch(value)
  {
    case NO_MARKING:
      return NO_FLAGS;

    case VISUAL:
      return EDGES | DASHES | IDENT;

    case NON_PAP:
      return EDGES | THRESHOLD | TOUCHDOWN | DASHES | IDENT;

    case PAP:
      return EDGES | THRESHOLD | FIXED_DISTANCE | TOUCHDOWN | DASHES | IDENT | PRECISION;

    case UK_NON_PAP:
      return EDGES | ALTERNATE_THRESHOLD | ALTERNATE_TOUCHDOWN | DASHES | IDENT;

    case UK_PAP:
      return EDGES | ALTERNATE_THRESHOLD | ALTERNATE_FIXEDDISTANCE | ALTERNATE_TOUCHDOWN | DASHES | IDENT | ALTERNATE_PRECISION;

    case EASA_NON_PAP:
      return EDGES | ALTERNATE_THRESHOLD | ALTERNATE_TOUCHDOWN | DASHES | IDENT;

    case EASA_PAP:
      return EDGES | ALTERNATE_THRESHOLD | ALTERNATE_FIXEDDISTANCE | ALTERNATE_TOUCHDOWN | DASHES | IDENT | ALTERNATE_PRECISION;
  }

  qWarning() << (context != nullptr ? context->messagePrefix() : QString()) << "Unknown markings value" << value;

  return NO_MARKING;
}

QString alsToDb(ApproachLight value, const XpReaderContext *context)
{
  switch(value)
  {
    case NO_ALS:
      return QString();

    case ALSF_I:
      return "ALSF1";

    case ALSF_II:
      return "ALSF2";

    case CALVERT:
      return "CALVERT";

    case CALVERT_ILS:
      return "CALVERT2";

    case SSALR:
      return "SSALR";

    case SSALF:
      return "SSALF";

    case SALS:
      return "SALS";

    case MALSR:
      return "MALSR";

    case MALSF:
      return "MALSF";

    case MALS:
      return "MALS";

    case ODALS:
      return "ODALS";

    case RAIL:
      return "RAIL";

  }
  qWarning() << (context != nullptr ? context->messagePrefix() : QString()) << "Unknown ALS value" << value;
  return QString();
}

QString approachIndicatorToDb(ApproachIndicator value, const XpReaderContext *context)
{
  // return "VASI21";
  // return "VASI22";
  // return "VASI31";
  // return "VASI32";
  // return "VASI23";
  // return "VASI33";
  // return "PAPI2";
  // return "PAPI4";
  // return "PVASI";
  // return "TVASI";
  // return "TRICOLOR";
  // return "BALL";
  // return "APAP_PANELS";

  switch(value)
  {
    case VASI:
      return "VASI22";

    case PAPI_4L:
    case PAPI_4R:
      return "PAPI4";

    case APAPI_L:
    case APAPI_R:
      return "APAPI";

    case SPACE_SHUTTLE_PAPI:
      return "PAPI4";

    case TRI_COLOR_VASI:
      return "TRICOLOR";

    case RUNWAY_GUARD:
      return "GUARD";

    case NO_APPR_INDICATOR:
      break;
  }
  qWarning() << (context != nullptr ? context->messagePrefix() : QString())
             << "Unknown ApproachIndicator value" << value;
  return QString();
}

QString XpReaderContext::messagePrefix() const
{
  if(fileVersion > 0)
    return QString("File %1, version %2, line %3").arg(filePath).arg(fileVersion).arg(lineNumber);
  else
    return QString("File %1, line %3").arg(filePath).arg(lineNumber);
}

} // namespace xp
} // namespace fs
} // namespace atools
