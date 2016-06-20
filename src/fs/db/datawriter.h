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

#ifndef ATOOLS_FS_DB_DATAWRITER_H
#define ATOOLS_FS_DB_DATAWRITER_H

namespace atools {
namespace sql {
class SqlDatabase;
}

namespace fs {
class BglReaderOptions;
namespace scenery {
class SceneryArea;
}

namespace db {

class BglFileWriter;
class SceneryAreaWriter;
class AirportWriter;
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
class TempAirwayWriter;
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

class DataWriter
{
public:
  DataWriter(atools::sql::SqlDatabase& sqlDb, const atools::fs::BglReaderOptions& opts,
             ProgressHandler *progress);

  virtual ~DataWriter();

  void writeSceneryArea(const atools::fs::scenery::SceneryArea& area);
  void logResults();

  void increaseNumObjects()
  {
    numObjectsWritten++;
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

  AirportIndex *getAirportIndex()
  {
    return airportIndex;
  }

  RunwayIndex *getRunwayIndex()
  {
    return runwayIndex;
  }

  atools::fs::db::SceneryAreaWriter *getSceneryAreaWriter()
  {
    return sceneryAreaWriter;
  }

  atools::fs::db::TempAirwayWriter *getTempAirwayWriter()
  {
    return tempAirwayWriter;
  }

  atools::fs::db::DeleteAirportWriter *getDeleteAirportWriter()
  {
    return deleteAirportWriter;
  }

  const BglReaderOptions& getOptions() const
  {
    return options;
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

  bool isAborted() const
  {
    return aborted;
  }

private:
  int numFiles = 0, numAirports = 0, numNamelists = 0, numVors = 0, numIls = 0,
      numNdbs = 0, numMarker = 0, numWaypoints = 0, numBoundaries = 0, numObjectsWritten = 0;
  bool aborted = false;

  atools::sql::SqlDatabase& db;
  atools::fs::db::ProgressHandler *progressHandler;

  atools::fs::db::BglFileWriter *bglFileWriter;
  atools::fs::db::SceneryAreaWriter *sceneryAreaWriter;

  atools::fs::db::AirportWriter *airportWriter;
  atools::fs::db::RunwayWriter *runwayWriter;
  atools::fs::db::RunwayEndWriter *runwayEndWriter;

  atools::fs::db::ApproachWriter *approachWriter;
  atools::fs::db::ApproachLegWriter *approachLegWriter;
  atools::fs::db::TransitionWriter *approachTransWriter;
  atools::fs::db::TransitionLegWriter *approachTransLegWriter;

  atools::fs::db::ParkingWriter *parkingWriter;
  atools::fs::db::ComWriter *airportComWriter;
  atools::fs::db::HelipadWriter *airportHelipadWriter;
  atools::fs::db::StartWriter *airportStartWriter;
  atools::fs::db::ApronWriter *airportApronWriter;
  atools::fs::db::ApronLightWriter *airportApronLightWriter;
  atools::fs::db::FenceWriter *airportFenceWriter;
  atools::fs::db::TaxiPathWriter *airportTaxiPathWriter;

  atools::fs::db::DeleteAirportWriter *deleteAirportWriter;

  atools::fs::db::WaypointWriter *waypointWriter;
  atools::fs::db::TempAirwayWriter *tempAirwayWriter;
  atools::fs::db::VorWriter *vorWriter;
  atools::fs::db::NdbWriter *ndbWriter;
  atools::fs::db::MarkerWriter *markerWriter;
  atools::fs::db::IlsWriter *ilsWriter;

  atools::fs::db::BoundaryWriter *boundaryWriter;
  atools::fs::db::BoundaryLineWriter *boundaryLineWriter;

  atools::fs::db::RunwayIndex *runwayIndex;
  atools::fs::db::AirportIndex *airportIndex;

  const atools::fs::BglReaderOptions& options;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_DATAWRITER_H
