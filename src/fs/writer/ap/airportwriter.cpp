/*
 * AirportWriter.cpp
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#include "fs/writer/ap/airportwriter.h"
#include "fs/writer/meta/bglfilewriter.h"
#include "fs/writer/ap/rw/runwaywriter.h"
#include "fs/writer/ap/approachwriter.h"
#include "fs/writer/ap/parkingwriter.h"
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
  if(!getOptions().doesAirportIcaoMatch(type->getIdent()))
    return;

  if(getOptions().isVerbose())
    qDebug() << "Writing airport " << type->getIdent() << " name " << type->getName();

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
  bind(":num_helipads", type->getNumHelipads());
  bind(":has_avgas", (type->getFuelFlags() & AVGAS) == AVGAS);
  bind(":has_jetfuel", (type->getFuelFlags() & JET_FUEL) == JET_FUEL);
  bind(":has_boundary_fence", type->hasBoundaryFence());
  bind(":has_tower_object", type->hasTowerObj());
  bind(":has_taxiways", type->hasTaxiway());
  bind(":has_apron", type->hasApron());
  bind(":has_jetways", type->hasJetway());
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
    if(!(getOptions().filterRunways() &&
         rwy.getLength() <= MIN_RUNWAY_LENGTH &&
         rwy.getSurface() == atools::fs::bgl::rw::GRASS))
      rwWriter->writeOne(rwy);

  WaypointWriter *waypointWriter = getDataWriter().getWaypointWriter();
  waypointWriter->write(type->getWaypoints());

  ComWriter *comWriter = getDataWriter().getAirportComWriter();
  comWriter->write(type->getComs());

  ApproachWriter *appWriter = getDataWriter().getApproachWriter();
  appWriter->write(type->getApproaches());

  ParkingWriter *parkWriter = getDataWriter().getParkingWriter();
  parkWriter->write(type->getParkings());

  DeleteAirportWriter *deleteAirportWriter = getDataWriter().getDeleteAirportWriter();

  const QList<DeleteAirport>& deleteAirports = type->getDeleteAirports();
  for(DeleteAirport delAp : deleteAirports)
  {
    deleteAirportWriter->writeOne(delAp);
    if(getOptions().execDeletes())
      deleteProcessor.processDelete(delAp, type, getCurrentId());
  }
}

} // namespace writer
} // namespace fs
} // namespace atools
