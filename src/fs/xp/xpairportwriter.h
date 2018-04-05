/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FS_XP_AIRPORTWRITER_H
#define ATOOLS_FS_XP_AIRPORTWRITER_H

#include "fs/xp/xpwriter.h"

#include "geo/rect.h"
#include "fs/xp/xpconstants.h"
#include "fs/common/xpgeometry.h"
#include "sql/sqlrecord.h"

#include <QApplication>

namespace atools {

namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {

namespace common {
class AirportIndex;
}

class NavDatabaseOptions;
class ProgressHandler;
class NavDatabaseErrors;

namespace xp {

/*
 * Reads one or more airports from an apt.dat file and writes them into a database
 */
class XpAirportWriter :
  public atools::fs::xp::XpWriter
{
  Q_DECLARE_TR_FUNCTIONS(XpAirportWriter)

public:
  XpAirportWriter(atools::sql::SqlDatabase& sqlDb, atools::fs::common::AirportIndex *airportIndexParam,
                  const atools::fs::NavDatabaseOptions& opts, atools::fs::ProgressHandler *progressHandler,
                  atools::fs::NavDatabaseErrors *navdatabaseErrors);
  virtual ~XpAirportWriter();

  virtual void write(const QStringList& line, const XpWriterContext& context) override;
  virtual void finish(const XpWriterContext& context) override;

  virtual void reset() override;

private:
  void initQueries();
  void deInitQueries();

  /* Fill airport data from the header into the query */
  void bindAirport(const QStringList& line, atools::fs::xp::AirportRowCode airportRowCode,
                   const XpWriterContext& context);

  /* Add metadata from key/value pairs */
  void bindMetadata(const QStringList& line, const XpWriterContext& context);

  /* Add viewpoint as tower position */
  void bindViewpoint(const QStringList& line, const XpWriterContext& context);

  /* Add fuel flags from truck parking positions */
  void bindFuel(const QStringList& line, const XpWriterContext& context);

  /* Finalize and write airport */
  void finishAirport(const XpWriterContext& context);

  /* Collect runway data and write start positions */
  void bindRunway(const QStringList& line, AirportRowCode airportRowCode, const XpWriterContext& context);

  /* Write helipad and start positions */
  void writeHelipad(const QStringList& line, const XpWriterContext& context);

  /* TWR, ASOS, ATIS, etc. */
  void writeCom(const QStringList& line, AirportRowCode rowCode, const XpWriterContext& context);

  /* Add vasi to runway end */
  void bindVasi(const QStringList& line, const XpWriterContext& context);

  /* File metadata for lookup in GUI*/
  void writeAirportFile(const QString& icao, int curFileId);

  /* Start pavement (taxi and apron) by header */
  void bindPavement(const QStringList& line, const atools::fs::xp::XpWriterContext& context);
  void bindPavementNode(const QStringList& line, atools::fs::xp::AirportRowCode rowCode,
                        const XpWriterContext& context);
  void finishPavement(const XpWriterContext& context);

  /* Obsolete type 15 */
  void writeStartup(const QStringList& line, const XpWriterContext& context);

  /* Write parking */
  void writeStartupLocation(const QStringList& line, const XpWriterContext& context);
  void writeStartupLocationMetadata(const QStringList& line, const XpWriterContext& context);
  void finishStartupLocation();

  /* Collect taxi nodes (not written) */
  void bindTaxiNode(const QStringList& line, const XpWriterContext& context);

  /* Write taxi edges */
  void bindTaxiEdge(const QStringList& line, const XpWriterContext& context);

  int compareGate(const QString& gate1, const QString& gate2);
  int compareRamp(const QString& ramp1, const QString& ramp2);

  /* State information */
  bool writingAirport = false, ignoringAirport = false,
       writingPavementBoundary = false, writingPavementHoles = false, writingPavementNewHole = false,
       writingStartLocation = false, airportClosed = false;

  /* Current feature ids */
  int curAirportId = 0, curRunwayId = 0, curRunwayEndId = 0, curHelipadId = 0, curComId = 0, curStartId = 0,
      curParkingId = 0, curApronId = 0, curTaxiPathId = 0, curHelipadStartNumber = 0,
      curAirportFileId = 10000000 /* Needs to count down since reading order is reversed */;

  bool hasTower = false;

  /* Counters for redundant airport data */
  int numRunwayEndAls = 0, numRunwayEndIls = 0, numHardRunway = 0, numApron = 0,
      numRunwayEndClosed = 0, numSoftRunway = 0, numRunway = 0, numWaterRunway = 0, numLightRunway = 0, numHelipad = 0,
      numCom = 0, numStart = 0, numParking = 0, numTaxiPath = 0,
      numRunwayEndVasi = 0, numJetway = 0, numBoundaryFence = 0,
      numParkingGaRamp = 0, numParkingGate = 0, numParkingCargo = 0,
      numParkingMilitaryCombat = 0, numParkingMilCargo = 0, numParkingMilCombat = 0;

  atools::sql::SqlRecordVector runwayEndRecords;
  /* pre-filled record */
  const atools::sql::SqlRecord runwayEndRecord;

  float airportAltitude = 0.f;
  float longestRunwayLength = 0.f, longestRunwayWidth = 0.f, longestRunwayHeading = 0.f;

  QString longestRunwaySurface = "UNKNOWN";
  atools::geo::Pos longestRunwayCenterPos;

  AirportRowCode airportRowCode = NO_ROWCODE;

  atools::sql::SqlQuery *insertAirportQuery = nullptr,
                        *insertRunwayQuery = nullptr, *insertRunwayEndQuery = nullptr, *insertHelipadQuery = nullptr,
                        *insertComQuery = nullptr, *insertApronQuery = nullptr, *insertTaxiQuery = nullptr,
                        *insertStartQuery = nullptr, *insertParkingQuery = nullptr, *insertAirportFileQuery = nullptr;

  QString largestParkingRamp, largestParkingGate;
  QString airportIcao;

  atools::geo::Rect airportRect;
  atools::geo::Pos airportPos, airportDatumPos;
  atools::fs::common::AirportIndex *airportIndex;
  atools::fs::common::XpGeometry currentPavement;
  QHash<int, atools::geo::Pos> taxiNodes;

};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_AIRPORTWRITER_H
