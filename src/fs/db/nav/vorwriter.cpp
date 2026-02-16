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

#include "fs/db/nav/vorwriter.h"

#include "atools.h"
#include "fs/bgl/nav/dme.h"
#include "fs/db/datawriter.h"
#include "fs/db/meta/bglfilewriter.h"
#include "fs/util/tacanfrequencies.h"
#include "geo/calculations.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Dme;
using atools::fs::bgl::Vor;
using namespace atools::geo;
using namespace atools;

void VorWriter::writeObject(const Vor *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing VOR " << type->getIdent() << type->getName();

  if(type->getIdent().isEmpty())
  {
    qWarning() << Q_FUNC_INFO << "Found VOR with empty ident in file"
               << getDataWriter().getBglFileWriter()->getCurrentFilepath();
    return;
  }

  bind(QStringLiteral(":vor_id"), getNextId());
  bind(QStringLiteral(":file_id"), getDataWriter().getBglFileWriter()->getCurrentId());
  bind(QStringLiteral(":ident"), type->getIdent());
  bind(QStringLiteral(":name"), type->getName());
  bind(QStringLiteral(":region"), type->getRegion());

  if(type->isVortac()) // Only MSFS - P3D uses TacanWriter
    bind(QStringLiteral(":type"), QStringLiteral("VT%1").arg(bgl::IlsVor::ilsVorTypeToStr(type->getType())));
  else if(type->isTacan()) // Only MFSF - P3D uses TacanWriter
    bind(QStringLiteral(":type"), QStringLiteral("TC"));
  else
    bind(QStringLiteral(":type"), bgl::IlsVor::ilsVorTypeToStr(type->getType()));

  bind(QStringLiteral(":airport_ident"), type->getAirportIdent());
  bindNullInt(QStringLiteral(":airport_id"));

  bind(QStringLiteral(":frequency"), type->getFrequency());

  if(type->isTacan() || type->isVortac()) // Only MFSF 2024
    bind(QStringLiteral(":channel"), util::tacanChannelForFrequency(type->getFrequency() / 10));
  else
    bindNullString(QStringLiteral(":channel"));

  bind(QStringLiteral(":range"), roundToInt(meterToNm(type->getRange())));

  if(type->isDmeOnly())
    bind(QStringLiteral(":mag_var"), getDataWriter().getMagVar(type->getPosition().getPos(), type->getMagVar()));
  else
    bind(QStringLiteral(":mag_var"), type->getMagVar());

  bind(QStringLiteral(":dme_only"), type->isDmeOnly());
  bind(QStringLiteral(":altitude"), roundToInt(meterToFeet(type->getPosition().getAltitude())));
  bind(QStringLiteral(":lonx"), type->getPosition().getLonX());
  bind(QStringLiteral(":laty"), type->getPosition().getLatY());

  const Dme *dme = type->getDme();
  if(dme != nullptr)
  {
    bind(QStringLiteral(":dme_altitude"), roundToInt(meterToFeet(dme->getPosition().getAltitude())));
    bind(QStringLiteral(":dme_lonx"), dme->getPosition().getLonX());
    bind(QStringLiteral(":dme_laty"), dme->getPosition().getLatY());
  }
  else
  {
    bindNullFloat(QStringLiteral(":dme_altitude"));
    bindNullFloat(QStringLiteral(":dme_lonx"));
    bindNullFloat(QStringLiteral(":dme_laty"));
  }
  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
