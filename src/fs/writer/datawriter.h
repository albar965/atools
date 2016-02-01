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

#ifndef WRITER_DATAWRITER_H_
#define WRITER_DATAWRITER_H_

namespace atools {
namespace sql {
class SqlDatabase;
}

namespace fs {
class BglReaderOptions;
namespace scenery {
class SceneryArea;
}

namespace writer {

class BglFileWriter;
class SceneryAreaWriter;
class AirportWriter;
class RunwayWriter;
class RunwayEndWriter;
class ApproachWriter;
class TransitionWriter;
class ParkingWriter;
class ComWriter;
class DeleteAirportWriter;
class WaypointWriter;
class TempRouteWriter;
class VorWriter;
class NdbWriter;
class MarkerWriter;
class IlsWriter;
class StartWriter;
class HelipadWriter;
class RunwayIndex;
class AirportIndex;

class DataWriter
{
public:
  DataWriter(atools::sql::SqlDatabase& sqlDb, const atools::fs::BglReaderOptions& opts);

  virtual ~DataWriter();

  void writeSceneryArea(const atools::fs::scenery::SceneryArea& area);
  void logResults();

  void increaseNumObjects()
  {
    numObjectsWritten++;
  }

  atools::fs::writer::BglFileWriter *getBglFileWriter()
  {
    return bglFileWriter;
  }

  atools::fs::writer::AirportWriter *getAirportWriter()
  {
    return airportWriter;
  }

  atools::fs::writer::WaypointWriter *getWaypointWriter()
  {
    return waypointWriter;
  }

  atools::fs::writer::ComWriter *getAirportComWriter()
  {
    return airportComWriter;
  }

  atools::fs::writer::RunwayWriter *getRunwayWriter()
  {
    return runwayWriter;
  }

  atools::fs::writer::RunwayEndWriter *getRunwayEndWriter()
  {
    return runwayEndWriter;
  }

  atools::fs::writer::TransitionWriter *getApproachTransWriter()
  {
    return approachTransWriter;
  }

  atools::fs::writer::ApproachWriter *getApproachWriter()
  {
    return approachWriter;
  }

  atools::fs::writer::ParkingWriter *getParkingWriter()
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

  atools::fs::writer::SceneryAreaWriter *getSceneryAreaWriter()
  {
    return sceneryAreaWriter;
  }

  atools::fs::writer::TempRouteWriter *getTempRouteWriter()
  {
    return tempRouteWriter;
  }

  atools::fs::writer::DeleteAirportWriter *getDeleteAirportWriter()
  {
    return deleteAirportWriter;
  }

  const BglReaderOptions& getOptions() const
  {
    return options;
  }

  atools::fs::writer::HelipadWriter *getHelipadWriter() const
  {
    return airportHelipadWriter;
  }

  atools::fs::writer::StartWriter *getStartWriter() const
  {
    return airportStartWriter;
  }

private:
  int numFiles = 0, numAirports = 0, numNamelists = 0, numVors = 0, numIls = 0,
      numNdbs = 0, numMarker = 0, numWaypoints = 0, numObjectsWritten = 0;

  atools::sql::SqlDatabase& db;

  atools::fs::writer::BglFileWriter *bglFileWriter;
  atools::fs::writer::SceneryAreaWriter *sceneryAreaWriter;

  atools::fs::writer::AirportWriter *airportWriter;
  atools::fs::writer::RunwayWriter *runwayWriter;
  atools::fs::writer::RunwayEndWriter *runwayEndWriter;
  atools::fs::writer::ApproachWriter *approachWriter;
  atools::fs::writer::TransitionWriter *approachTransWriter;
  atools::fs::writer::ParkingWriter *parkingWriter;
  atools::fs::writer::ComWriter *airportComWriter;
  atools::fs::writer::HelipadWriter *airportHelipadWriter;
  atools::fs::writer::StartWriter *airportStartWriter;
  atools::fs::writer::DeleteAirportWriter *deleteAirportWriter;

  atools::fs::writer::WaypointWriter *waypointWriter;
  atools::fs::writer::TempRouteWriter *tempRouteWriter;
  atools::fs::writer::VorWriter *vorWriter;
  atools::fs::writer::NdbWriter *ndbWriter;
  atools::fs::writer::MarkerWriter *markerWriter;
  atools::fs::writer::IlsWriter *ilsWriter;

  atools::fs::writer::RunwayIndex *runwayIndex;
  atools::fs::writer::AirportIndex *airportIndex;

  const atools::fs::BglReaderOptions& options;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_DATAWRITER_H_ */
