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

#include "fs/db/nav/ilswriter.h"
#include "fs/bgl/nav/dme.h"
#include "fs/bgl/nav/glideslope.h"
#include "fs/bgl/nav/localizer.h"
#include "fs/bgl/util.h"
#include "fs/db/datawriter.h"
#include "sql/sqlquery.h"
#include "fs/navdatabaseoptions.h"
#include "fs/db/runwayindex.h"
#include "geo/calculations.h"
#include "atools.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Dme;
using atools::fs::bgl::Glideslope;
using atools::fs::bgl::Localizer;
using atools::fs::bgl::Ils;
using atools::geo::Pos;

void IlsWriter::writeObject(const Ils *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing ILS " << type->getIdent() << " name " << type->getName();

  bind(":ils_id", getNextId());
  bind(":ident", type->getIdent());
  bind(":name", type->getName());
  bind(":region", type->getRegion());
  bind(":frequency", type->getFrequency());
  bind(":range", roundToInt(atools::geo::meterToNm(type->getRange())));
  bind(":mag_var", type->getMagVar());
  bind(":has_backcourse", type->hasBackcourse());
  bind(":altitude", roundToInt(atools::geo::meterToFeet(type->getPosition().getAltitude())));

  const bgl::BglPosition& pos = type->getPosition();
  const Localizer *loc = type->getLocalizer();

  if(loc != nullptr)
  {
    int length = atools::geo::nmToMeter(FEATHER_LEN_NM);
    // Calculate the display of the ILS feather
    float ilsHeading = atools::geo::normalizeCourse(atools::geo::opposedCourseDeg(loc->getHeading()));
    Pos p1 = pos.getPos().endpoint(length, ilsHeading - loc->getWidth() / 2).normalize();
    Pos p2 = pos.getPos().endpoint(length, ilsHeading + loc->getWidth() / 2).normalize();
    float featherWidth = p1.distanceMeterTo(p2);
    Pos pmid = pos.getPos().endpoint(length - featherWidth / 2, ilsHeading).normalize();

    bind(":end1_lonx", p1.getLonX());
    bind(":end1_laty", p1.getLatY());

    bind(":end_mid_lonx", pmid.getLonX());
    bind(":end_mid_laty", pmid.getLatY());

    bind(":end2_lonx", p2.getLonX());
    bind(":end2_laty", p2.getLatY());
  }

  bind(":lonx", pos.getLonX());
  bind(":laty", pos.getLatY());

  const Dme *dme = type->getDme();
  if(dme != nullptr)
  {
    bind(":dme_range", roundToInt(atools::geo::meterToNm(dme->getRange())));
    bind(":dme_altitude", roundToInt(atools::geo::meterToFeet(dme->getPosition().getAltitude())));
    bind(":dme_lonx", dme->getPosition().getLonX());
    bind(":dme_laty", dme->getPosition().getLatY());
  }
  else
  {
    bindNullInt(":dme_range");
    bindNullInt(":dme_altitude");
    bindNullFloat(":dme_lonx");
    bindNullFloat(":dme_laty");
  }

  const Glideslope *gs = type->getGlideslope();
  if(gs != nullptr)
  {
    bind(":gs_range", roundToInt(atools::geo::meterToNm(gs->getRange())));
    bind(":gs_pitch", gs->getPitch());
    bind(":gs_altitude", roundToInt(atools::geo::meterToFeet(gs->getPosition().getAltitude())));
    bind(":gs_lonx", gs->getPosition().getLonX());
    bind(":gs_laty", gs->getPosition().getLatY());
  }
  else
  {
    bindNullInt(":gs_range");
    bindNullFloat(":gs_pitch");
    bindNullInt(":gs_altitude");
    bindNullFloat(":gs_lonx");
    bindNullFloat(":gs_laty");
  }

  bool isComplete = false;
  QString apIdent = type->getAirportIdent();

  bindNullInt(":loc_runway_end_id");
  bindNullFloat(":loc_heading");
  bindNullFloat(":loc_width");

  if(!apIdent.isEmpty())
    bind(":loc_airport_ident", apIdent);
  else
    bindNullString(":loc_airport_ident");

  if(loc != nullptr)
  {
    bind(":loc_runway_name", loc->getRunwayName());
    bind(":loc_heading", loc->getHeading());
    bind(":loc_width", loc->getWidth());
  }
  else
    isComplete = true;

  if(getOptions().isIncomplete() || isComplete)
    executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
