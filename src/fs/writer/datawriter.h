/*
 * WriterList.h
 *
 *  Created on: 02.05.2015
 *      Author: alex
 */

#ifndef WRITER_DATAWRITER_H_
#define WRITER_DATAWRITER_H_

#include "nav/waypointwriter.h"
#include "nav/temproutewriter.h"
#include "nav/vorwriter.h"
#include "nav/ndbwriter.h"
#include "nav/markerwriter.h"
#include "nav/ilswriter.h"
#include "meta/bglfilewriter.h"
#include "ap/airportwriter.h"
#include "ap/rw/runwaywriter.h"
#include "ap/rw/runwayendwriter.h"
#include "runwayindex.h"
#include "airportindex.h"
#include "ap/approachwriter.h"
#include "ap/comwriter.h"
#include "ap/transitionwriter.h"
#include "ap/parkingwriter.h"
#include "ap/deleteairportwriter.h"

#include "../scenery/fileresolver.h"
#include "meta/sceneryareawriter.h"

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
class DataWriter
{
public:
  DataWriter(atools::sql::SqlDatabase& sqlDb, const BglReaderOptions& opts);

  virtual ~DataWriter()
  {
  }

  void writeSceneryArea(const scenery::SceneryArea& area);
  void logResults();

  void increaseNumObjects()
  {
    numObjectsWritten++;
  }

  atools::fs::writer::BglFileWriter& getBglFileWriter()
  {
    return bglFileWriter;
  }

  atools::fs::writer::AirportWriter& getAirportWriter()
  {
    return airportWriter;
  }

  atools::fs::writer::WaypointWriter& getWaypointWriter()
  {
    return waypointWriter;
  }

  atools::fs::writer::ComWriter& getAirportComWriter()
  {
    return airportComWriter;
  }

  atools::fs::writer::RunwayWriter& getRunwayWriter()
  {
    return runwayWriter;
  }

  atools::fs::writer::RunwayEndWriter& getRunwayEndWriter()
  {
    return runwayEndWriter;
  }

  atools::fs::writer::TransitionWriter& getApproachTransWriter()
  {
    return approachTransWriter;
  }

  atools::fs::writer::ApproachWriter& getApproachWriter()
  {
    return approachWriter;
  }

  atools::fs::writer::ParkingWriter& getParkingWriter()
  {
    return parkingWriter;
  }

  AirportIndex& getAirportIndex()
  {
    return airportIndex;
  }

  RunwayIndex& getRunwayIndex()
  {
    return runwayIndex;
  }

  atools::fs::writer::SceneryAreaWriter& getSceneryAreaWriter()
  {
    return sceneryAreaWriter;
  }

  atools::fs::writer::TempRouteWriter& getTempRouteWriter()
  {
    return tempRouteWriter;
  }

  atools::fs::writer::DeleteAirportWriter& getDeleteAirportWriter()
  {
    return deleteAirportWriter;
  }

  const BglReaderOptions& getOptions() const
  {
    return options;
  }

private:
  int numFiles, numAirports, numNamelists, numVors, numIls, numNdbs, numMarker, numWaypoints,
      numObjectsWritten;

  atools::fs::scenery::FileResolver resolver;
  atools::sql::SqlDatabase& db;

  atools::fs::writer::BglFileWriter bglFileWriter;
  atools::fs::writer::SceneryAreaWriter sceneryAreaWriter;

  atools::fs::writer::AirportWriter airportWriter;
  atools::fs::writer::RunwayWriter runwayWriter;
  atools::fs::writer::RunwayEndWriter runwayEndWriter;
  atools::fs::writer::ApproachWriter approachWriter;
  atools::fs::writer::TransitionWriter approachTransWriter;
  atools::fs::writer::ParkingWriter parkingWriter;
  atools::fs::writer::ComWriter airportComWriter;
  atools::fs::writer::DeleteAirportWriter deleteAirportWriter;

  atools::fs::writer::WaypointWriter waypointWriter;
  atools::fs::writer::TempRouteWriter tempRouteWriter;
  atools::fs::writer::VorWriter vorWriter;
  atools::fs::writer::NdbWriter ndbWriter;
  atools::fs::writer::MarkerWriter markerWriter;
  atools::fs::writer::IlsWriter ilsWriter;

  RunwayIndex runwayIndex;
  AirportIndex airportIndex;
  const BglReaderOptions& options;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_DATAWRITER_H_ */
