/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include "fs/writer/ap/airportwriter.h"
#include "fs/writer/meta/bglfilewriter.h"
#include "fs/writer/ap/rw/runwaywriter.h"
#include "fs/writer/ap/approachwriter.h"
#include "fs/writer/ap/startwriter.h"
#include "fs/writer/ap/helipadwriter.h"
#include "fs/writer/ap/parkingwriter.h"
#include "fs/writer/ap/apronwriter.h"
#include "fs/writer/ap/apronlightwriter.h"
#include "fs/writer/ap/taxipathwriter.h"
#include "fs/writer/ap/fencewriter.h"
#include "fs/bgl/nl/namelist.h"
#include "fs/bgl/nl/namelistentry.h"
#include "fs/writer/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/bgl/ap/rw/runway.h"
#include "fs/bgl/ap/del/deleteairport.h"
#include "fs/writer/airportindex.h"
#include "fs/writer/nav/waypointwriter.h"
#include "fs/writer/ap/comwriter.h"
#include "fs/writer/ap/deleteairportwriter.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::ap::AVGAS;
using atools::fs::bgl::ap::JET_FUEL;
using atools::fs::bgl::Namelist;
using atools::fs::bgl::NamelistEntry;
using atools::fs::bgl::Com;
using atools::fs::bgl::Airport;
using atools::fs::bgl::Runway;
using atools::fs::bgl::Apron;
using atools::fs::bgl::Apron2;
using atools::fs::bgl::ApronLight;
using atools::fs::bgl::Fence;
using atools::sql::SqlQuery;
using atools::fs::bgl::DeleteAirport;

void AirportWriter::setNameLists(const QList<const Namelist *>& namelists)
{
  for(const Namelist *iter : namelists)
    for(const NamelistEntry &i : iter->getNameList())
      nameListIndex[i.getAirportIdent()] = &i;
}

void AirportWriter::writeObject(const Airport *type)
{
  if(!getOptions().includeAirport(type->getIdent()))
    return;

  qDebug() << "Writing airport" << type->getIdent() << "name" << type->getName();

  bindNullString(":country");
  bindNullString(":state");
  bindNullString(":city");

  NameListMapConstIterType it = nameListIndex.find(type->getIdent());
  if(it != nameListIndex.end())
  {
    const NamelistEntry *nl = it.value();
    if(nl != nullptr)
    {
      bind(":country", nl->getCountryName());
      bind(":state", nl->getStateName());
      bind(":city", nl->getCityName());
    }
    else
      qWarning().nospace().noquote() << "NameEntry for airport " << type->getIdent() << " is null";
  }
  else
    qWarning().nospace().noquote() << "NameEntry for airport " << type->getIdent() << " not found";

  bind(":airport_id", getNextId());
  bind(":file_id", getDataWriter().getBglFileWriter()->getCurrentId());
  bind(":ident", type->getIdent());
  bind(":region", type->getRegion());
  bind(":name", type->getName());
  bind(":fuel_flags", type->getFuelFlags());
  bind(":has_avgas", ((type->getFuelFlags() & AVGAS) == AVGAS) ? 1 : 0);
  bind(":has_jetfuel", ((type->getFuelFlags() & JET_FUEL) == JET_FUEL) ? 1 : 0);
  bind(":has_tower_object", type->hasTowerObj() ? 1 : 0);
  bind(":mag_var", type->getMagVar());
  bind(":tower_lonx", type->getTowerPosition().getLonX());
  bind(":tower_laty", type->getTowerPosition().getLatY());
  bind(":altitude", bgl::util::meterToFeet(type->getPosition().getAltitude()));
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  executeStatement();

  currentIdent = type->getIdent();
  getAirportIndex()->add(type->getIdent(), getCurrentId());

  RunwayWriter *rwWriter = getDataWriter().getRunwayWriter();

  const QList<Runway>& runways = type->getRunways();

  for(Runway rwy : runways)
    if(!(getOptions().isFilterRunways() &&
         rwy.getLength() <= MIN_RUNWAY_LENGTH && rwy.getSurface() == atools::fs::bgl::rw::GRASS))
      rwWriter->writeOne(rwy);

  WaypointWriter *waypointWriter = getDataWriter().getWaypointWriter();
  waypointWriter->write(type->getWaypoints());

  ComWriter *comWriter = getDataWriter().getAirportComWriter();
  comWriter->write(type->getComs());

  ApproachWriter *appWriter = getDataWriter().getApproachWriter();
  appWriter->write(type->getApproaches());

  HelipadWriter *heliWriter = getDataWriter().getHelipadWriter();
  heliWriter->write(type->getHelipads());

  StartWriter *startWriter = getDataWriter().getStartWriter();
  startWriter->write(type->getStarts());

  ParkingWriter *parkWriter = getDataWriter().getParkingWriter();
  parkWriter->write(type->getParkings());

  ApronWriter *apronWriter = getDataWriter().getApronWriter();
  const QList<bgl::Apron>& aprons = type->getAprons();
  const QList<bgl::Apron2>& aprons2 = type->getAprons2();
  for(int i = 0; i < aprons.size(); i++)
  {
    QPair<const Apron *, const Apron2 *> pair(&aprons.at(i), &aprons2.at(i));
    apronWriter->writeOne(pair);
  }

  ApronLightWriter *apronLightWriter = getDataWriter().getApronLightWriter();
  apronLightWriter->write(type->getApronsLights());

  FenceWriter *fenceWriter = getDataWriter().getFenceWriter();
  fenceWriter->write(type->getFences());

  TaxiPathWriter *taxiWriter = getDataWriter().getTaxiPathWriter();
  taxiWriter->write(type->getTaxiPaths());

  DeleteAirportWriter *deleteAirportWriter = getDataWriter().getDeleteAirportWriter();

  const QList<DeleteAirport>& deleteAirports = type->getDeleteAirports();
  for(const DeleteAirport& delAp : deleteAirports)
  {
    deleteAirportWriter->writeOne(delAp);
    if(getOptions().isDeletes())
      deleteProcessor.processDelete(&delAp, type, getCurrentId());
  }
}

} // namespace writer
} // namespace fs
} // namespace atools
