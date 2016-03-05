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

#include "fs/db/ap/airportwriter.h"
#include "fs/db/meta/bglfilewriter.h"
#include "fs/db/meta/sceneryareawriter.h"
#include "fs/db/ap/rw/runwaywriter.h"
#include "fs/db/ap/approachwriter.h"
#include "fs/db/ap/startwriter.h"
#include "fs/db/ap/helipadwriter.h"
#include "fs/db/ap/parkingwriter.h"
#include "fs/db/ap/apronwriter.h"
#include "fs/db/ap/apronlightwriter.h"
#include "fs/db/ap/taxipathwriter.h"
#include "fs/db/ap/fencewriter.h"
#include "fs/bgl/nl/namelist.h"
#include "fs/bgl/nl/namelistentry.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/util.h"
#include "geo/calculations.h"
#include "fs/bgl/ap/rw/runway.h"
#include "fs/bgl/ap/del/deleteairport.h"
#include "fs/db/airportindex.h"
#include "fs/db/nav/waypointwriter.h"
#include "fs/db/ap/comwriter.h"
#include "fs/db/ap/deleteairportwriter.h"
#include "fs/bgl/ap/parking.h"

namespace atools {
namespace fs {
namespace db {

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
  using namespace atools::geo;

  if(!getOptions().includeAirport(type->getIdent()))
    return;

  if(getOptions().isVerbose())
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

  DataWriter& dw = getDataWriter();
  bind(":airport_id", getNextId());
  bind(":file_id", dw.getBglFileWriter()->getCurrentId());
  bind(":ident", type->getIdent());
  bind(":name", type->getName());
  bind(":fuel_flags", type->getFuelFlags());
  bind(":has_avgas", (type->getFuelFlags() & AVGAS) == AVGAS);
  bindBool(":has_jetfuel", (type->getFuelFlags() & JET_FUEL) == JET_FUEL);
  bindBool(":has_tower_object", type->hasTowerObj());

  bindIntOrNull(":tower_frequency", type->getTowerFrequency());
  bindIntOrNull(":atis_frequency", type->getAtisFrequency());
  bindIntOrNull(":awos_frequency", type->getAwosFrequency());
  bindIntOrNull(":asos_frequency", type->getAsosFrequency());
  bindIntOrNull(":unicom_frequency", type->getUnicomFrequency());

  bindBool(":is_closed", type->isAirportClosed());
  bindBool(":is_military", type->isMilitary());
  bool isAddon = getOptions().isAddonPath(dw.getSceneryAreaWriter()->getCurrentSceneryLocalPath());
  bindBool(":is_addon", isAddon);

  bind(":num_boundary_fence", type->getNumBoundaryFence());
  bind(":num_com", type->getComs().size());

  bind(":num_parking_gate", type->getNumParkingGate());
  bind(":num_parking_ga_ramp", type->getNumParkingGaRamp());

  bind(":num_parking_cargo", type->getNumParkingCargo());
  bind(":num_parking_mil_cargo", type->getNumParkingMilitaryCargo());
  bind(":num_parking_mil_combat", type->getNumParkingMilitaryCombat());

  bind(":num_approach", type->getApproaches().size());
  bind(":num_runway_soft", type->getNumSoftRunway());
  bind(":num_runway_hard", type->getNumHardRunway());
  bind(":num_runway_water", type->getNumWaterRunway());
  bind(":num_runway_light", type->getNumLightRunway());
  bind(":num_runway_end_closed", type->getNumRunwayEndClosed());
  bind(":num_runway_end_vasi", type->getNumRunwayEndVasi());
  bind(":num_runway_end_als", type->getNumRunwayEndApproachLight());
  bind(":num_apron", type->getAprons().size());
  bind(":num_taxi_path", type->getTaxiPaths().size());
  bind(":num_helipad", type->getHelipads().size());
  bind(":num_jetway", type->getNumJetway());
  bindNullInt(":num_runway_end_ils"); // Will be set later by SQL script

  bind(":longest_runway_length", roundToPrecision(meterToFeet(type->getLongestRunwayLength())));
  bind(":longest_runway_width", roundToPrecision(meterToFeet(type->getLongestRunwayWidth())));
  bind(":longest_runway_heading", type->getLongestRunwayHeading());
  bind(":longest_runway_surface", Runway::surfaceToStr(type->getLongestRunwaySurface()));

  bind(":num_runways", type->getRunways().size());

  bind(":largest_parking_ramp",
       bgl::util::enumToStr(bgl::Parking::parkingTypeToStr, type->getLargestParkingGaRamp()));
  bind(":largest_parking_gate",
       bgl::util::enumToStr(bgl::Parking::parkingTypeToStr, type->getLargestParkingGate()));

  bind(":rating", !type->getTaxiPaths().isEmpty() + type->hasTowerObj() + !type->getParkings().isEmpty() +
       !type->getAprons().isEmpty() + isAddon);

  bind(":scenery_local_path", dw.getSceneryAreaWriter()->getCurrentSceneryLocalPath());
  bind(":bgl_filename", dw.getBglFileWriter()->getCurrentFilename());

  bind(":left_lonx", type->getBoundingRect().getTopLeft().getLonX());
  bind(":top_laty", type->getBoundingRect().getTopLeft().getLatY());
  bind(":right_lonx", type->getBoundingRect().getBottomRight().getLonX());
  bind(":bottom_laty", type->getBoundingRect().getBottomRight().getLatY());

  bind(":mag_var", type->getMagVar());
  bind(":tower_altitude", roundToPrecision(meterToFeet(type->getPosition().getAltitude())));
  bind(":tower_lonx", type->getTowerPosition().getLonX());
  bind(":tower_laty", type->getTowerPosition().getLatY());
  bind(":altitude", roundToPrecision(meterToFeet(type->getPosition().getAltitude())));
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  executeStatement();

  currentIdent = type->getIdent();
  getAirportIndex()->add(type->getIdent(), getCurrentId());

  RunwayWriter *rwWriter = dw.getRunwayWriter();
  rwWriter->write(type->getRunways());

  WaypointWriter *waypointWriter = dw.getWaypointWriter();
  waypointWriter->write(type->getWaypoints());

  ComWriter *comWriter = dw.getAirportComWriter();
  comWriter->write(type->getComs());

  ApproachWriter *appWriter = dw.getApproachWriter();
  appWriter->write(type->getApproaches());

  HelipadWriter *heliWriter = dw.getHelipadWriter();
  heliWriter->write(type->getHelipads());

  StartWriter *startWriter = dw.getStartWriter();
  startWriter->write(type->getStarts());

  ParkingWriter *parkWriter = dw.getParkingWriter();
  parkWriter->write(type->getParkings());

  ApronWriter *apronWriter = dw.getApronWriter();
  const QList<bgl::Apron>& aprons = type->getAprons();
  const QList<bgl::Apron2>& aprons2 = type->getAprons2();
  for(int i = 0; i < aprons.size(); i++)
  {
    QPair<const Apron *, const Apron2 *> pair(&aprons.at(i), &aprons2.at(i));
    apronWriter->writeOne(pair);
  }

  ApronLightWriter *apronLightWriter = dw.getApronLightWriter();
  apronLightWriter->write(type->getApronsLights());

  FenceWriter *fenceWriter = dw.getFenceWriter();
  fenceWriter->write(type->getFences());

  TaxiPathWriter *taxiWriter = dw.getTaxiPathWriter();
  taxiWriter->write(type->getTaxiPaths());

  DeleteAirportWriter *deleteAirportWriter = dw.getDeleteAirportWriter();

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
