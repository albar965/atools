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

#ifndef ATOOLS_FS_DB_DATAWRITER_H
#define ATOOLS_FS_DB_DATAWRITER_H

#include <QList>
#include <QSet>
#include <QString>

#include "fs/navdatabaseerrors.h"

namespace atools {
namespace sql {
class SqlDatabase;
}

namespace fs {
class NavDatabaseOptions;
class NavDatabaseErrors;
namespace scenery {
class SceneryArea;
}

namespace db {

class BglFileWriter;
class SceneryAreaWriter;
class AirportWriter;
class AirportFileWriter;
class RunwayWriter;
class RunwayEndWriter;
class ApproachWriter;
class ApproachLegWriter;
class TransitionWriter;
class TransitionLegWriter;
class ParkingWriter;
class ComWriter;
class DeleteAirportWriter;
class WaypointWriter;
class AirwaySegmentWriter;
class VorWriter;
class NdbWriter;
class MarkerWriter;
class IlsWriter;
class StartWriter;
class HelipadWriter;
class RunwayIndex;
class AirportIndex;
class ApronWriter;
class ApronLightWriter;
class FenceWriter;
class TaxiPathWriter;
class BoundaryWriter;
class BoundaryLineWriter;
class ProgressHandler;

/*
 * Keeps all writer objects and calls them in order to write BGL records to the database.
 */
class DataWriter
{
public:
  DataWriter(atools::sql::SqlDatabase& sqlDb, const atools::fs::NavDatabaseOptions& opts,
             atools::fs::db::ProgressHandler *progress);
  virtual ~DataWriter();

  /*
   * @param area all BGL file content of this scenery area will be written to the database
   */
  void writeSceneryArea(const atools::fs::scenery::SceneryArea& area);

  /*
   * Log written record number, etc. to the log/console.
   */
  void logResults();

  void increaseNumObjects()
  {
    numObjectsWritten++;
  }

  /*
   * @return true if the progress callback reported an abort (i.e. Cancel button pressed)
   */
  bool isAborted() const
  {
    return aborted;
  }

  /*
   * @return airport index that maps ICAO idents to database airport Ids
   */
  AirportIndex *getAirportIndex()
  {
    return airportIndex;
  }

  /*
   * @return runway index that maps runway names to database runway end Ids
   */
  RunwayIndex *getRunwayIndex()
  {
    return runwayIndex;
  }

  /*
   * @return configuration options for the scenery library compiler
   */
  const NavDatabaseOptions& getOptions() const
  {
    return options;
  }

  atools::fs::db::BglFileWriter *getBglFileWriter()
  {
    return bglFileWriter;
  }

  atools::fs::db::AirportWriter *getAirportWriter()
  {
    return airportWriter;
  }

  atools::fs::db::WaypointWriter *getWaypointWriter()
  {
    return waypointWriter;
  }

  atools::fs::db::ComWriter *getAirportComWriter()
  {
    return airportComWriter;
  }

  atools::fs::db::RunwayWriter *getRunwayWriter()
  {
    return runwayWriter;
  }

  atools::fs::db::RunwayEndWriter *getRunwayEndWriter()
  {
    return runwayEndWriter;
  }

  atools::fs::db::TransitionLegWriter *getApproachTransLegWriter()
  {
    return approachTransLegWriter;
  }

  atools::fs::db::TransitionWriter *getApproachTransWriter()
  {
    return approachTransWriter;
  }

  atools::fs::db::ApproachLegWriter *getApproachLegWriter()
  {
    return approachLegWriter;
  }

  atools::fs::db::ApproachWriter *getApproachWriter()
  {
    return approachWriter;
  }

  atools::fs::db::ParkingWriter *getParkingWriter()
  {
    return parkingWriter;
  }

  atools::fs::db::SceneryAreaWriter *getSceneryAreaWriter()
  {
    return sceneryAreaWriter;
  }

  atools::fs::db::AirwaySegmentWriter *getAirwaySegmentWriter()
  {
    return airwaySegmentWriter;
  }

  atools::fs::db::DeleteAirportWriter *getDeleteAirportWriter()
  {
    return deleteAirportWriter;
  }

  atools::fs::db::HelipadWriter *getHelipadWriter() const
  {
    return airportHelipadWriter;
  }

  atools::fs::db::StartWriter *getStartWriter() const
  {
    return airportStartWriter;
  }

  atools::fs::db::ApronWriter *getApronWriter() const
  {
    return airportApronWriter;
  }

  atools::fs::db::ApronLightWriter *getApronLightWriter() const
  {
    return airportApronLightWriter;
  }

  atools::fs::db::FenceWriter *getFenceWriter() const
  {
    return airportFenceWriter;
  }

  atools::fs::db::TaxiPathWriter *getTaxiPathWriter() const
  {
    return airportTaxiPathWriter;
  }

  atools::fs::db::BoundaryWriter *getBoundaryWriter() const
  {
    return boundaryWriter;
  }

  atools::fs::db::BoundaryLineWriter *getBoundaryLineWriter() const
  {
    return boundaryLineWriter;
  }

  /*
   * Set the error list that will be filled whenever exceptions occur
   */
  void setSceneryErrors(atools::fs::NavDatabaseErrors::SceneryErrors *errors)
  {
    sceneryErrors = errors;
  }

  /* Close all writers and queries */
  void close();

private:
  int numFiles = 0, numNamelists = 0, numVors = 0, numIls = 0,
      numNdbs = 0, numMarker = 0, numWaypoints = 0, numBoundaries = 0, numObjectsWritten = 0;
  bool aborted = false;

  QSet<QString> airportIdents;

  atools::sql::SqlDatabase& db;
  atools::fs::db::ProgressHandler *progressHandler = nullptr;
  atools::fs::NavDatabaseErrors::SceneryErrors *sceneryErrors = nullptr;

  atools::fs::db::BglFileWriter *bglFileWriter = nullptr;
  atools::fs::db::SceneryAreaWriter *sceneryAreaWriter = nullptr;

  atools::fs::db::AirportWriter *airportWriter = nullptr;
  atools::fs::db::AirportFileWriter *airportFileWriter = nullptr;
  atools::fs::db::RunwayWriter *runwayWriter = nullptr;
  atools::fs::db::RunwayEndWriter *runwayEndWriter = nullptr;

  atools::fs::db::ApproachWriter *approachWriter = nullptr;
  atools::fs::db::ApproachLegWriter *approachLegWriter = nullptr;
  atools::fs::db::TransitionWriter *approachTransWriter = nullptr;
  atools::fs::db::TransitionLegWriter *approachTransLegWriter = nullptr;

  atools::fs::db::ParkingWriter *parkingWriter = nullptr;
  atools::fs::db::ComWriter *airportComWriter = nullptr;
  atools::fs::db::HelipadWriter *airportHelipadWriter = nullptr;
  atools::fs::db::StartWriter *airportStartWriter = nullptr;
  atools::fs::db::ApronWriter *airportApronWriter = nullptr;
  atools::fs::db::ApronLightWriter *airportApronLightWriter = nullptr;
  atools::fs::db::FenceWriter *airportFenceWriter = nullptr;
  atools::fs::db::TaxiPathWriter *airportTaxiPathWriter = nullptr;

  atools::fs::db::DeleteAirportWriter *deleteAirportWriter = nullptr;

  atools::fs::db::WaypointWriter *waypointWriter = nullptr;
  atools::fs::db::AirwaySegmentWriter *airwaySegmentWriter = nullptr;
  atools::fs::db::VorWriter *vorWriter = nullptr;
  atools::fs::db::NdbWriter *ndbWriter = nullptr;
  atools::fs::db::MarkerWriter *markerWriter = nullptr;
  atools::fs::db::IlsWriter *ilsWriter = nullptr;

  atools::fs::db::BoundaryWriter *boundaryWriter = nullptr;
  atools::fs::db::BoundaryLineWriter *boundaryLineWriter = nullptr;

  atools::fs::db::RunwayIndex *runwayIndex = nullptr;
  atools::fs::db::AirportIndex *airportIndex = nullptr;

  const atools::fs::NavDatabaseOptions& options;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_DATAWRITER_H
