/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#include "fs/navdatabaseerrors.h"

#include <QList>
#include <QSet>
#include <QString>
#include <QCoreApplication>

namespace atools {
namespace sql {
class SqlDatabase;
}
namespace geo {
class Pos;
}
namespace fs {
class NavDatabaseOptions;
class NavDatabaseErrors;
namespace common {
class MagDecReader;
}
namespace scenery {
class SceneryArea;
class LanguageJson;
class MaterialLib;
}
class ProgressHandler;

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
class SidStarWriter;
class SidStarApproachLegWriter;
class SidStarTransitionWriter;
class SidStarTransitionLegWriter;
class ParkingWriter;
class ComWriter;
class WaypointWriter;
class AirwaySegmentWriter;
class VorWriter;
class TacanWriter;
class NdbWriter;
class MarkerWriter;
class IlsWriter;
class StartWriter;
class HelipadWriter;
class RunwayIndex;
class ApronWriter;
class TaxiPathWriter;
class BoundaryWriter;

/*
 * Keeps all writer objects and calls them in order to write BGL records to the database.
 */
class DataWriter
{
  Q_DECLARE_TR_FUNCTIONS(DataWriter)

public:
  DataWriter(atools::sql::SqlDatabase& sqlDb, const atools::fs::NavDatabaseOptions& opts,
             atools::fs::ProgressHandler *progress);
  virtual ~DataWriter();

  DataWriter(const DataWriter& other) = delete;
  DataWriter& operator=(const DataWriter& other) = delete;

  /*
   * @param area all BGL file content of this scenery area will be written to the database
   */
  void writeSceneryArea(const atools::fs::scenery::SceneryArea& area);

  void readMagDeclBgl(const QString& fileScenery, bool forceWmm = false);

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

  atools::fs::db::SidStarTransitionLegWriter *getSidStarTransLegWriter()
  {
    return sidStarTransLegWriter;
  }

  atools::fs::db::SidStarTransitionWriter *getSidStarTransWriter()
  {
    return sidStarTransWriter;
  }

  atools::fs::db::SidStarApproachLegWriter *getSidStarApproachLegWriter()
  {
    return sidStarApproachLegWriter;
  }

  atools::fs::db::SidStarWriter *getSidStarWriter()
  {
    return sidStarWriter;
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

  atools::fs::db::TaxiPathWriter *getTaxiPathWriter() const
  {
    return airportTaxiPathWriter;
  }

  atools::fs::db::BoundaryWriter *getBoundaryWriter() const
  {
    return boundaryWriter;
  }

  atools::fs::db::VorWriter *getVorWriter() const
  {
    return vorWriter;
  }

  /*
   * Set the error list that will be filled whenever exceptions occur
   */
  void setSceneryErrors(atools::fs::SceneryErrors *errors)
  {
    sceneryErrors = errors;
  }

  /* Close all writers and queries */
  void close();

  float getMagVar(const atools::geo::Pos& pos, float defaultValue) const;

  /* MSFS language index from JSON file */
  QString getLanguage(const QString& key);

  /* MSFS surface type from material library */
  QString getSurface(const QUuid& key);

  /* Language index to look up localized MSFS airport and other names that start with "TT:" */
  void setLanguageIndex(const atools::fs::scenery::LanguageJson *value)
  {
    languageIndex = value;
  }

  /* Global MSFS material library */
  void setMaterialLib(const atools::fs::scenery::MaterialLib *value)
  {
    materialLib = value;
  }

  /* Package specific MSFS material library */
  void setMaterialLibScenery(const atools::fs::scenery::MaterialLib *value)
  {
    materialLibScenery = value;
  }

  atools::sql::SqlDatabase& getDatabase() const
  {
    return db;
  }

  /* Get next ids from writers */
  int getNextSceneryId() const;
  int getNextFileId() const;

private:
  int numFiles = 0, numNamelists = 0, numVors = 0, numIls = 0,
      numNdbs = 0, numMarker = 0, numWaypoints = 0, numBoundaries = 0, numObjectsWritten = 0;
  bool aborted = false;

  QSet<QString> airportIdents;

  atools::sql::SqlDatabase& db;
  atools::fs::ProgressHandler *progressHandler = nullptr;
  atools::fs::SceneryErrors *sceneryErrors = nullptr;

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

  atools::fs::db::SidStarWriter *sidStarWriter = nullptr;
  atools::fs::db::SidStarApproachLegWriter *sidStarApproachLegWriter = nullptr;
  atools::fs::db::SidStarTransitionWriter *sidStarTransWriter = nullptr;
  atools::fs::db::SidStarTransitionLegWriter *sidStarTransLegWriter = nullptr;

  atools::fs::db::ParkingWriter *parkingWriter = nullptr;
  atools::fs::db::ComWriter *airportComWriter = nullptr;
  atools::fs::db::HelipadWriter *airportHelipadWriter = nullptr;
  atools::fs::db::StartWriter *airportStartWriter = nullptr;
  atools::fs::db::ApronWriter *airportApronWriter = nullptr;
  atools::fs::db::TaxiPathWriter *airportTaxiPathWriter = nullptr;

  atools::fs::db::WaypointWriter *waypointWriter = nullptr;
  atools::fs::db::AirwaySegmentWriter *airwaySegmentWriter = nullptr;
  atools::fs::db::VorWriter *vorWriter = nullptr;
  atools::fs::db::TacanWriter *tacanWriter = nullptr;
  atools::fs::db::NdbWriter *ndbWriter = nullptr;
  atools::fs::db::MarkerWriter *markerWriter = nullptr;
  atools::fs::db::IlsWriter *ilsWriter = nullptr;

  atools::fs::db::BoundaryWriter *boundaryWriter = nullptr;

  atools::fs::db::RunwayIndex *runwayIndex = nullptr;
  atools::fs::common::MagDecReader *magDecReader = nullptr;

  const atools::fs::NavDatabaseOptions& options;
  const atools::fs::scenery::LanguageJson *languageIndex = nullptr;
  const atools::fs::scenery::MaterialLib *materialLib = nullptr, *materialLibScenery = nullptr;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_DATAWRITER_H
