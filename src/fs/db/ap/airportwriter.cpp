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

#include "fs/db/ap/airportwriter.h"
#include "fs/db/meta/bglfilewriter.h"
#include "fs/db/meta/sceneryareawriter.h"
#include "fs/db/ap/rw/runwaywriter.h"
#include "fs/db/ap/approachwriter.h"
#include "fs/db/ap/sidstarwriter.h"
#include "fs/db/ap/startwriter.h"
#include "fs/db/ap/helipadwriter.h"
#include "fs/db/ap/parkingwriter.h"
#include "fs/db/ap/apronwriter.h"
#include "fs/db/ap/taxipathwriter.h"
#include "fs/bgl/nl/namelist.h"
#include "fs/bgl/nl/namelistentry.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/util.h"
#include "geo/calculations.h"
#include "fs/bgl/ap/rw/runway.h"
#include "fs/bgl/ap/del/deleteairport.h"
#include "fs/db/nav/waypointwriter.h"
#include "fs/db/ap/comwriter.h"
#include "fs/bgl/ap/parking.h"
#include "fs/scenery/languagejson.h"
#include "fs/util/fsutil.h"
#include "atools.h"
#include "exception.h"

#include <QDir>
#include <QFileInfo>

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
using atools::fs::bgl::DeleteAirport;
using atools::fs::bgl::SidStar;
using atools::geo::meterToFeet;

AirportWriter::AirportWriter(sql::SqlDatabase& db, DataWriter& dataWriter)
  : WriterBase(db, dataWriter, "airport"), deleteProcessor(db, dataWriter.getOptions())
{
  query = new atools::sql::SqlQuery(getDataWriter().getDatabase());
  query->prepare("select airport_id from airport where ident = ?");
}

AirportWriter::~AirportWriter()
{
  delete query;
}

void AirportWriter::setNameLists(const QList<const Namelist *>& namelists)
{
  nameListIndex.clear();
  for(const Namelist *iter : namelists)
  {
    for(const NamelistEntry& i : iter->getNameList())
      nameListIndex[i.getAirportIdent()] = &i;
  }
}

void AirportWriter::writeObject(const Airport *type)
{
  QString ident = type->getIdent();

  if(!getOptions().isIncludedAirportIdent(ident))
    return;

  if(!type->isValid())
  {
    qWarning() << Q_FUNC_INFO << "Skipping invalid airport" << ident;
    return;
  }

  DataWriter& dw = getDataWriter();
  const SceneryAreaWriter *sceneryAreaWriter = dw.getSceneryAreaWriter();
  const scenery::SceneryArea& currentArea = sceneryAreaWriter->getCurrentArea();
  const NavDatabaseOptions& options = getOptions();

  bool msfsNavdata = currentArea.isNavdata();
  bool msfs = options.getSimulatorType() == atools::fs::FsPaths::MSFS;

  int predId = airportIdByIdent(ident, msfsNavdata /* warn */);

  if(ident.isEmpty())
    throw atools::Exception("Found airport without ident");

  BglFileWriter *bglFileWriter = dw.getBglFileWriter();
  if(msfsNavdata)
  {
    // =========================================================================
    // Do a shortcut for MSFS dummies which transport only procedures and COM
    // Instead of writing a new airport simply add COM and procedures

    // Get the other airport id and remember the current one
    int currentId = dw.getAirportWriter()->setCurrentId(predId);

    // Update index
    currentIdent = ident;
    currentPos = type->getPos();

    // Write features with other airport id
    ComWriter *comWriter = dw.getAirportComWriter();
    comWriter->write(type->getComs());

    ApproachWriter *appWriter = dw.getApproachWriter();
    appWriter->write(type->getApproaches());

    SidStarWriter *sidStarWriter = dw.getSidStarWriter();
    sidStarWriter->write(type->getSidsAndStars());

    updateMsfsAirport(type, predId);

    // reset airport id to current
    dw.getAirportWriter()->setCurrentId(currentId);
  }
  else
  {
    // Normal stock or add-on airport =========================================
    if(msfs && predId == -1 && type->isMsfsPoiDummy(currentArea.isCommunity() || currentArea.isIncluded()))
      // Skip empty POI dummy airports in MSFS but not if there is a stock airport with the same name
      return;

    bool realAddon = false, // Add-ons need deleteprocessor action
         addon = false; // This flag indicates if the airport is hightlighted as an add-on

    if(msfs)
      // MSFS add-on status is set in the scenery area
      realAddon = currentArea.isCommunity() || currentArea.isIncluded() || currentArea.isAddOn();
    else
      // Check if this is an addon airport - if yes start the delete processor
      // Airport is add-on if it is not in the default scenery and not excluded from add-on recognition
      realAddon = options.isAddonLocalPath(sceneryAreaWriter->getCurrentSceneryLocalPath());

    // This is the shown add-on status - can be changed by filter in GUI
    addon = options.isAddonGui(QFileInfo(bglFileWriter->getCurrentFilepath())) && realAddon;

    // Third party navdata update or MSFS stock airport in official - not an addon
    if(currentArea.isMsfsNavigraphNavdata())
      addon = false;

#ifdef DEBUG_INFORMATION
    if(realAddon && type->getDeleteAirports().isEmpty())
      qInfo() << "Addon airport without delete record" << ident;
#endif

    int nextAirportId = getNextId();

    const DeleteAirport *delAp = nullptr;
    if(!type->getDeleteAirports().isEmpty())
      delAp = &type->getDeleteAirports().constFirst();

    // Get country, state and city =====================
    QString city, state, country, region;
    fetchAdmin(type, city, state, country, region);
    QString name = atools::fs::util::capAirportName(getDataWriter().getLanguage(type->getName())).simplified();

    deleteProcessor.init(delAp, type, getCurrentId(), name, city, state, country, region);

    if(options.isDeletes())
    {
      if(delAp != nullptr || realAddon)
        // Now delete the stock/default airport
        deleteProcessor.preProcessDelete();
    }

    QStringList sceneryLocalPaths, bglFilenames;
    if(!deleteProcessor.getSceneryLocalPath().isEmpty())
      sceneryLocalPaths.append(atools::nativeCleanPath(deleteProcessor.getSceneryLocalPath()));
    if(!deleteProcessor.getBglFilename().isEmpty())
      bglFilenames.append(deleteProcessor.getBglFilename());

    sceneryLocalPaths.append(atools::nativeCleanPath(dw.getSceneryAreaWriter()->getCurrentSceneryLocalPath()));
    bglFilenames.append(bglFileWriter->getCurrentFilename());

    // Write admin names =====================
    bindStrOrNull(":name", name);
    bindStrOrNull(":city", city);
    bindStrOrNull(":state", state);
    bindStrOrNull(":country", country);
    bindStrOrNull(":region", region);

    bind(":airport_id", nextAirportId);
    bind(":file_id", bglFileWriter->getCurrentId());
    bind(":ident", ident);
    bindNullString(":icao");
    bindNullString(":iata");
    bindNullString(":faa");
    bindNullString(":local");

    bind(":fuel_flags", type->getFuelFlags());

    bindBool(":has_avgas", (type->getFuelFlags() & AVGAS) == AVGAS);
    bindBool(":has_jetfuel", (type->getFuelFlags() & JET_FUEL) == JET_FUEL);

    bindBool(":has_tower_object", type->hasTowerObj());

    int towerFrequency = 0, unicomFrequency = 0, awosFrequency = 0, asosFrequency = 0, atisFrequency = 0;
    Airport::extractMainComFrequencies(type->getComs(), towerFrequency, unicomFrequency, awosFrequency, asosFrequency, atisFrequency);
    bindIntOrNull(":tower_frequency", towerFrequency);
    bindIntOrNull(":atis_frequency", atisFrequency);
    bindIntOrNull(":awos_frequency", awosFrequency);
    bindIntOrNull(":asos_frequency", asosFrequency);
    bindIntOrNull(":unicom_frequency", unicomFrequency);

    bindBool(":is_closed", type->isAirportClosed());

    bindBool(":is_military", atools::fs::util::isNameMilitary(name));

    bindBool(":is_addon", addon);
    bindBool(":is_3d", 0);

    bind(":num_com", type->getComs().size());

    bind(":num_parking_gate", type->getNumParkingGate());
    bind(":num_parking_ga_ramp", type->getNumParkingGaRamp());

    bind(":num_parking_cargo", type->getNumParkingCargo());
    bind(":num_parking_mil_cargo", type->getNumParkingMilitaryCargo());
    bind(":num_parking_mil_combat", type->getNumParkingMilitaryCombat());

    bind(":num_approach", type->getApproaches().size());

    int numHard = 0, numSoft = 0, numWater = 0;
    for(const Runway& rw : type->getRunways())
    {
      atools::fs::bgl::Surface surface = rw.getSurface();
      if(!rw.getMaterialUuid().isNull())
        surface = atools::fs::bgl::surface::surfaceToType(getDataWriter().getSurface(rw.getMaterialUuid()));

      numHard += atools::fs::bgl::surface::isHard(surface);
      numSoft += atools::fs::bgl::surface::isSoft(surface);
      numWater += atools::fs::bgl::surface::isWater(surface);
    }

    bind(":num_runway_soft", numSoft);
    bind(":num_runway_hard", numHard);
    bind(":num_runway_water", numWater);

    bind(":num_runway_light", type->getNumLightRunway());
    bind(":num_runway_end_closed", type->getNumRunwayEndClosed());
    bind(":num_runway_end_vasi", type->getNumRunwayEndVasi());
    bind(":num_runway_end_als", type->getNumRunwayEndApproachLight());
    bind(":num_apron", type->getAprons().size());
    bind(":num_taxi_path", type->getTaxiPaths().size());
    bind(":num_helipad", type->getHelipads().size());
    bind(":num_jetway", type->getNumJetway());
    bind(":num_starts", type->getStarts().size());
    bindNullInt(":num_runway_end_ils"); // Will be set later by SQL script "update_ils_ids.sql"

    bind(":longest_runway_length", roundToInt(meterToFeet(type->getLongestRunwayLength())));
    bind(":longest_runway_width", roundToInt(meterToFeet(type->getLongestRunwayWidth())));
    bind(":longest_runway_heading", type->getLongestRunwayHeading());

    using atools::fs::bgl::surface::surfaceToDbStr;
    // Use MSFS material library is UUID is set
    if(!type->getLongestRunwayMaterialUuid().isNull())
      bind(":longest_runway_surface", surfaceToDbStr(getDataWriter().getSurface(type->getLongestRunwayMaterialUuid())));
    else
      bind(":longest_runway_surface", surfaceToDbStr(type->getLongestRunwaySurface()));

    bind(":num_runways", type->getRunways().size());

    bind(":largest_parking_ramp",
         bgl::util::enumToStr(bgl::Parking::parkingTypeToStr, type->getLargestParkingGaRamp()));
    bind(":largest_parking_gate", bgl::util::enumToStr(bgl::Parking::parkingTypeToStr, type->getLargestParkingGate()));

    // Star get full rating and are not updated later - others are set later
    bind(":rating", type->isMsfsStar() ? 5 : 0);

    bind(":scenery_local_path", sceneryLocalPaths.join(", "));
    bind(":bgl_filename", bglFilenames.join(", "));

    bind(":left_lonx", type->getBoundingRect().getTopLeft().getLonX());
    bind(":top_laty", type->getBoundingRect().getTopLeft().getLatY());
    bind(":right_lonx", type->getBoundingRect().getBottomRight().getLonX());
    bind(":bottom_laty", type->getBoundingRect().getBottomRight().getLatY());

    // Use magvar from BGL and fallback to airport included magvar (usually inaccurate)
    bind(":mag_var", getDataWriter().getMagVar(type->getPos(), type->getMagVar()));

    if(!type->getTowerPosition().getPos().isNull() && type->getTowerPosition().getPos().isValidRange() &&
       !type->getTowerPosition().getPos().isPole())
    {
      bind(":tower_altitude", roundToInt(meterToFeet(type->getPos().getAltitude())));
      bind(":tower_lonx", type->getTowerPosition().getLonX());
      bind(":tower_laty", type->getTowerPosition().getLatY());
    }
    else
    {
      bindNullFloat(":tower_altitude");
      bindNullFloat(":tower_lonx");
      bindNullFloat(":tower_laty");
    }

    bind(":altitude", roundToInt(meterToFeet(type->getPos().getAltitude())));
    bind(":lonx", type->getPos().getLonX());
    bind(":laty", type->getPos().getLatY());

    // Write the airport to the database
    executeStatement();

    // Update index
    currentIdent = ident;
    currentPos = type->getPos();

    // Write all subrecords now since the airport id is not available - this keeps the foreign keys valid
    RunwayWriter *rwWriter = dw.getRunwayWriter();
    rwWriter->write(type->getRunways());

    if(!currentArea.isMsfsNavigraphNavdata())
    {
      WaypointWriter *waypointWriter = dw.getWaypointWriter();
      waypointWriter->write(type->getWaypoints());

      ComWriter *comWriter = dw.getAirportComWriter();
      comWriter->write(type->getComs());

      ApproachWriter *appWriter = dw.getApproachWriter();
      appWriter->write(type->getApproaches());

      SidStarWriter *sidStarWriter = dw.getSidStarWriter();
      sidStarWriter->write(type->getSidsAndStars());
    }

    // Helipads will take the start index + the current start id as reference to starts
    HelipadWriter *heliWriter = dw.getHelipadWriter();
    heliWriter->write(type->getHelipads());

    // Starts have to be written after helipads
    StartWriter *startWriter = dw.getStartWriter();
    startWriter->write(type->getStarts());

    ParkingWriter *parkWriter = dw.getParkingWriter();
    parkWriter->write(type->getParkings());

    // Apron writer needs two records
    ApronWriter *apronWriter = dw.getApronWriter();
    const QList<bgl::Apron>& aprons = type->getAprons();
    const QList<bgl::Apron2>& aprons2 = type->getAprons2();
    for(int i = 0; i < aprons.size(); i++)
      apronWriter->writeOne(std::make_pair(&aprons.at(i), aprons2.isEmpty() ? nullptr : &aprons2.at(i)));

    TaxiPathWriter *taxiWriter = dw.getTaxiPathWriter();
    taxiWriter->write(type->getTaxiPaths());

    if(options.isDeletes() && (delAp != nullptr || realAddon))
      // Now delete the stock/default/prev airport if there is any
      deleteProcessor.postProcessDelete();
  }
}

int atools::fs::db::AirportWriter::airportIdByIdent(const QString& ident, bool warn)
{
  query->bindValue(0, ident);
  query->exec();
  int newId = -1;
  if(query->next())
    newId = query->valueInt(0);

#ifdef DEBUG_INFORMATION
  if(newId == -1 && warn)
    qWarning() << Q_FUNC_INFO << "Other airport with ident" << ident << "not found";
#else
  Q_UNUSED(warn)
#endif
  return newId;
}

void AirportWriter::updateMsfsAirport(const Airport *type, int predId)
{
  int towerFrequency = 0, unicomFrequency = 0, awosFrequency = 0, asosFrequency = 0, atisFrequency = 0;
  Airport::extractMainComFrequencies(type->getComs(), towerFrequency, unicomFrequency, awosFrequency, asosFrequency,
                                     atisFrequency);

  int sidAndStarApproachCount = 0;
  for(const SidStar& sidStar : type->getSidsAndStars())
  {
    // Use the number of runway transitions as the "approach" count.
    // If it's 0, the procedure applies to all runways, and counts as 1.
    int rwtCnt = sidStar.getRunwayTransitionLegs().size();
    if(rwtCnt == 0)
      sidAndStarApproachCount += 1;
    else
      sidAndStarApproachCount += rwtCnt;
  }

  atools::sql::SqlQuery updateQuery(getDataWriter().getDatabase());
  updateQuery.prepare("update airport set tower_frequency = :tower_frequency, atis_frequency = :atis_frequency, "
                      "awos_frequency = :awos_frequency, asos_frequency = :asos_frequency, "
                      "unicom_frequency = :unicom_frequency, num_com = :num_com, num_approach = :num_approach "
                      // "lonx = :lonx, laty = :laty, "
                      // "left_lonx = :left_lonx, top_laty = :top_laty, "
                      // "right_lonx = :right_lonx, bottom_laty = :bottom_laty "
                      "where airport_id = :id");

  if(towerFrequency == 0)
    updateQuery.bindNullInt(":tower_frequency");
  else
    updateQuery.bindValue(":tower_frequency", towerFrequency);

  if(atisFrequency == 0)
    updateQuery.bindNullInt(":atis_frequency");
  else
    updateQuery.bindValue(":atis_frequency", atisFrequency);

  if(awosFrequency == 0)
    updateQuery.bindNullInt(":awos_frequency");
  else
    updateQuery.bindValue(":awos_frequency", awosFrequency);

  if(asosFrequency == 0)
    updateQuery.bindNullInt(":asos_frequency");
  else
    updateQuery.bindValue(":asos_frequency", asosFrequency);

  if(unicomFrequency == 0)
    updateQuery.bindNullInt(":unicom_frequency");
  else
    updateQuery.bindValue(":unicom_frequency", unicomFrequency);

  updateQuery.bindValue(":num_com", type->getComs().size());
  updateQuery.bindValue(":num_approach", type->getApproaches().size() + sidAndStarApproachCount);

  // query.bindValue(":lonx", type->getPosition().getLonX());
  // query.bindValue(":laty", type->getPosition().getLatY());
  // query.bindValue(":left_lonx", type->getBoundingRect().getWest());
  // query.bindValue(":top_laty", type->getBoundingRect().getNorth());
  // query.bindValue(":right_lonx", type->getBoundingRect().getEast());
  // query.bindValue(":bottom_laty", type->getBoundingRect().getSouth());

  updateQuery.bindValue(":id", predId);
  updateQuery.exec();
}

void AirportWriter::fetchAdmin(const Airport *type, QString& city, QString& state, QString& country, QString& region)
{
  const static QSet<QString> TOLOWER({"of", "and"});

  const NamelistEntry *nl = nameListIndex.value(type->getIdent(), nullptr);
  if(nl != nullptr)
  {
    city = atools::capString(getDataWriter().getLanguage(nl->getCityName()), {}, TOLOWER).simplified();
    state = atools::capString(getDataWriter().getLanguage(nl->getStateName()), {}, TOLOWER).simplified();
    country = atools::capString(getDataWriter().getLanguage(nl->getCountryName()), {}, TOLOWER).simplified();

    if(!type->getRegion().isEmpty())
      region = type->getRegion().simplified();
    else if(!nl->getRegionIdent().isEmpty())
      region = nl->getRegionIdent().simplified();
  }
}

} // namespace writer
} // namespace fs
} // namespace atools
