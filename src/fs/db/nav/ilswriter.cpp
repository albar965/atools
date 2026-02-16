/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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
#include "fs/db/datawriter.h"
#include "fs/util/fsutil.h"
#include "fs/navdatabaseoptions.h"
#include "fs/db/meta/bglfilewriter.h"
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

  if(type->getIdent().isEmpty())
  {
    qWarning() << Q_FUNC_INFO << "Found ILS with empty ident in file"
               << getDataWriter().getBglFileWriter()->getCurrentFilepath();
    return;
  }

  QString name = type->getName().trimmed();
  bind(QStringLiteral(":ils_id"), getNextId());
  bind(QStringLiteral(":ident"), type->getIdent().trimmed());
  bind(QStringLiteral(":name"), name);
  bind(QStringLiteral(":region"), type->getRegion());
  bind(QStringLiteral(":type"), type->getType());
  bind(QStringLiteral(":frequency"), type->getFrequency());
  bind(QStringLiteral(":range"), roundToInt(atools::geo::meterToNm(type->getRange())));
  bind(QStringLiteral(":mag_var"), type->getMagVar());
  bind(QStringLiteral(":has_backcourse"), type->hasBackcourse());
  bind(QStringLiteral(":altitude"), roundToInt(atools::geo::meterToFeet(type->getPosition().getAltitude())));

  const bgl::BglPosition& pos = type->getPosition();
  const Localizer *loc = type->getLocalizer();
  float headingTrue = 0.f;

  if(loc != nullptr)
  {
    if(getOptions().getSimulatorType() == atools::fs::FsPaths::MSFS)
      // MSFS uses magnetic course - turn to true
      headingTrue = atools::geo::normalizeCourse(loc->getHeading() + type->getMagVar());
    else
      // FSX and P3D use true course
      headingTrue = loc->getHeading();

    Pos p1, p2, pmid;
    atools::fs::util::calculateIlsGeometry(pos.getPos(), headingTrue, loc->getWidth(), atools::fs::util::DEFAULT_FEATHER_LEN_NM,
                                           p1, p2, pmid);

    bind(QStringLiteral(":end1_lonx"), p1.getLonX());
    bind(QStringLiteral(":end1_laty"), p1.getLatY());

    bind(QStringLiteral(":end_mid_lonx"), pmid.getLonX());
    bind(QStringLiteral(":end_mid_laty"), pmid.getLatY());

    bind(QStringLiteral(":end2_lonx"), p2.getLonX());
    bind(QStringLiteral(":end2_laty"), p2.getLatY());
  }

  bind(QStringLiteral(":lonx"), pos.getLonX());
  bind(QStringLiteral(":laty"), pos.getLatY());

  const Dme *dme = type->getDme();
  if(dme != nullptr)
  {
    bind(QStringLiteral(":dme_range"), roundToInt(atools::geo::meterToNm(dme->getRange())));
    bind(QStringLiteral(":dme_altitude"), roundToInt(atools::geo::meterToFeet(dme->getPosition().getAltitude())));
    bind(QStringLiteral(":dme_lonx"), dme->getPosition().getLonX());
    bind(QStringLiteral(":dme_laty"), dme->getPosition().getLatY());
  }
  else
  {
    bindNullInt(QStringLiteral(":dme_range"));
    bindNullInt(QStringLiteral(":dme_altitude"));
    bindNullFloat(QStringLiteral(":dme_lonx"));
    bindNullFloat(QStringLiteral(":dme_laty"));
  }

  const Glideslope *gs = type->getGlideslope();
  if(gs != nullptr)
  {
    bind(QStringLiteral(":gs_range"), roundToInt(atools::geo::meterToNm(gs->getRange())));
    bind(QStringLiteral(":gs_pitch"), gs->getPitch());
    bind(QStringLiteral(":gs_altitude"), roundToInt(atools::geo::meterToFeet(gs->getPosition().getAltitude())));
    bind(QStringLiteral(":gs_lonx"), gs->getPosition().getLonX());
    bind(QStringLiteral(":gs_laty"), gs->getPosition().getLatY());
  }
  else
  {
    bindNullInt(QStringLiteral(":gs_range"));
    bindNullFloat(QStringLiteral(":gs_pitch"));
    bindNullInt(QStringLiteral(":gs_altitude"));
    bindNullFloat(QStringLiteral(":gs_lonx"));
    bindNullFloat(QStringLiteral(":gs_laty"));
  }

  bindNullInt(QStringLiteral(":loc_runway_end_id"));
  bindNullFloat(QStringLiteral(":loc_heading"));
  bindNullFloat(QStringLiteral(":loc_width"));

  QString apIdent = type->getAirportIdent();
  if(!apIdent.isEmpty())
    bind(QStringLiteral(":loc_airport_ident"), apIdent);
  else
    bindNullString(QStringLiteral(":loc_airport_ident"));

  bool isComplete = false;
  if(loc != nullptr)
  {
    QString locName = loc->getRunwayName().trimmed();

    // Try to extract the runway number from the name if invalid ===========================
    if((getOptions().getSimulatorType() == atools::fs::FsPaths::MSFS ||
        getOptions().getSimulatorType() == atools::fs::FsPaths::MSFS_2024) &&
       (locName.isEmpty() || locName == QStringLiteral("0") || locName == QStringLiteral("00")))
    {
      // "IGS RWY 13", "ILS 01", "ILS 32", "ILS 32R", "ILS CAT III RWY 05R", "ILS CAT III RWY 23", "ILS RW01", "ILS RW01C",
      // "ILS RW01L", "ILS RW01R", "ILS RW36L", "ILS RW36R", "ILS RWY 05", "ILS RWY 05L", "ILS RWY 15", "ILS RWY 31", "ILS04",
      // "ILS08L", "ILSZ22R", "ILSZ4L", "LOC RWY 33", "ils runway 06", "ils runway 24"
      locName =
        name.toUpper().simplified().remove(QStringLiteral("IGS")).remove(QStringLiteral("ILSZ")).remove(QStringLiteral("ILSX")).remove(
          QStringLiteral("ILSY")).remove(QStringLiteral("ILS")).remove(QStringLiteral("CAT")).
        remove(QStringLiteral("I")).remove(QStringLiteral("II")).remove(QStringLiteral("III")).remove(QStringLiteral("LOC")).remove(
          QStringLiteral("RUNWAY")).remove(QStringLiteral("RWY")).remove(QStringLiteral("RW")).remove(' ');

      if(!atools::fs::util::runwayNameValid(locName))
        locName.clear();
    }

    if(!locName.isEmpty())
      bind(QStringLiteral(":loc_runway_name"), locName);
    else
      bindNullString(QStringLiteral(":loc_runway_name"));

    bind(QStringLiteral(":loc_heading"), headingTrue);
    bind(QStringLiteral(":loc_width"), loc->getWidth());
  }
  else
    isComplete = true;

  if(getOptions().isIncomplete() || isComplete)
    executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
