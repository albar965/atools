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

#include "fs/xp/xpconstants.h"

#include "fs/bgl/ap/rw/runway.h"

namespace atools {
namespace fs {
namespace xp {

QString surfaceToDb(Surface value, const XpWriterContext *context)
{
  switch(value)
  {
    case atools::fs::xp::UNKNOWN:
      return "UNKNOWN";

    case atools::fs::xp::TRANSPARENT:
      return "TR";

    case atools::fs::xp::ASPHALT:
      return "A";

    case atools::fs::xp::CONCRETE:
      return "C";

    case atools::fs::xp::TURF_OR_GRASS:
      return "G";

    case atools::fs::xp::DRY_LAKEBED:
    case atools::fs::xp::DIRT:
      return "D";

    case atools::fs::xp::GRAVEL:
      return "GR";

    case atools::fs::xp::WATER:
      return "W";

    case atools::fs::xp::SNOW_OR_ICE:
      return "SN";

  }
  // New unknown in XP12 20 22 23 24 25 26 27 29 30 31 33 34 36 37 38 50 51 52 53 54 55 56 57

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
  return value == UNKNOWN || value == TRANSPARENT || value == ASPHALT || value == CONCRETE;
}

bool isSurfaceSoft(Surface value)
{
  return value == TURF_OR_GRASS || value == DRY_LAKEBED || value == DIRT || value == GRAVEL || value == SNOW_OR_ICE;
}

bool isSurfaceWater(Surface value)
{
  return value == WATER;
}

int markingToDb(Marking value, const XpWriterContext *context)
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
    case atools::fs::xp::NO_MARKING:
      return NO_FLAGS;

    case atools::fs::xp::VISUAL:
      return EDGES | DASHES | IDENT;

    case atools::fs::xp::NON_PAP:
      return EDGES | THRESHOLD | TOUCHDOWN | DASHES | IDENT;

    case atools::fs::xp::PAP:
      return EDGES | THRESHOLD | FIXED_DISTANCE | TOUCHDOWN | DASHES | IDENT | PRECISION;

    case atools::fs::xp::UK_NON_PAP:
      return EDGES | ALTERNATE_THRESHOLD | ALTERNATE_TOUCHDOWN | DASHES | IDENT;

    case atools::fs::xp::UK_PAP:
      return EDGES | ALTERNATE_THRESHOLD | ALTERNATE_FIXEDDISTANCE | ALTERNATE_TOUCHDOWN | DASHES | IDENT |
             ALTERNATE_PRECISION;

  }
  qWarning() << (context != nullptr ? context->messagePrefix() : QString()) << "Unknown markings value" << value;
  return NO_MARKING;
}

QString alsToDb(ApproachLight value, const XpWriterContext *context)
{
  switch(value)
  {
    case atools::fs::xp::NO_ALS:
      return QString();

    case atools::fs::xp::ALSF_I:
      return "ALSF1";

    case atools::fs::xp::ALSF_II:
      return "ALSF2";

    case atools::fs::xp::CALVERT:
      return "CALVERT";

    case atools::fs::xp::CALVERT_ILS:
      return "CALVERT2";

    case atools::fs::xp::SSALR:
      return "SSALR";

    case atools::fs::xp::SSALF:
      return "SSALF";

    case atools::fs::xp::SALS:
      return "SALS";

    case atools::fs::xp::MALSR:
      return "MALSR";

    case atools::fs::xp::MALSF:
      return "MALSF";

    case atools::fs::xp::MALS:
      return "MALS";

    case atools::fs::xp::ODALS:
      return "ODALS";

    case atools::fs::xp::RAIL:
      return "RAIL";

  }
  qWarning() << (context != nullptr ? context->messagePrefix() : QString()) << "Unknown ALS value" << value;
  return QString();
}

QString approachIndicatorToDb(ApproachIndicator value, const XpWriterContext *context)
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
    case atools::fs::xp::VASI:
      return "VASI22";

    case atools::fs::xp::PAPI_4L:
      return "PAPI4";

    case atools::fs::xp::PAPI_4R:
      return "PAPI4";

    case atools::fs::xp::SPACE_SHUTTLE_PAPI:
      return "PAPI4";

    case atools::fs::xp::TRI_COLOR_VASI:
      return "TRICOLOR";

    case atools::fs::xp::RUNWAY_GUARD:
      return "GUARD";

    case atools::fs::xp::NO_APPR_INDICATOR:
      break;
  }
  qWarning() << (context != nullptr ? context->messagePrefix() : QString())
             << "Unknown ApproachIndicator value" << value;
  return QString();
}

QString XpWriterContext::messagePrefix() const
{
  if(fileVersion > 0)
    return QString("File %1, version %2, line %3").arg(filePath).arg(fileVersion).arg(lineNumber);
  else
    return QString("File %1, line %3").arg(filePath).arg(lineNumber);
}

} // namespace xp
} // namespace fs
} // namespace atools
