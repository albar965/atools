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

#ifndef ATOOLS_FS_XP_AIRPORTREADER_h
#define ATOOLS_FS_XP_AIRPORTREADER_h

#include "fs/xp/xpreader.h"

#include "geo/rect.h"
#include "geo/line.h"
#include "fs/common/xpgeometry.h"
#include "sql/sqlrecord.h"
#include "sql/sqltypes.h"

#include <QCoreApplication>

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

struct RunwayEnds;
struct RunwayGeometry;
struct RunwayDimension;

/*
 * Reads one or more airports from an apt.dat file and writes them into a database
 */
class XpAirportReader :
  public atools::fs::xp::XpReader
{
  Q_DECLARE_TR_FUNCTIONS(XpAirportWriter)

public:
  XpAirportReader(atools::sql::SqlDatabase& sqlDb, atools::fs::common::AirportIndex *airportIndexParam,
                  const atools::fs::NavDatabaseOptions& opts, atools::fs::ProgressHandler *progressHandler,
                  atools::fs::NavDatabaseErrors *navdatabaseErrors);
  virtual ~XpAirportReader() override;

  XpAirportReader(const XpAirportReader& other) = delete;
  XpAirportReader& operator=(const XpAirportReader& other) = delete;

  virtual void read(const QStringList& line, const XpReaderContext& context) override;
  virtual void finish(const XpReaderContext& context) override;

  virtual void reset() override;

private:
  void initQueries();
  void deInitQueries();

  /* Fill airport data from the header into the query */
  void bindAirport(const QStringList& line, atools::fs::xp::AirportRowCode airportRowCode,
                   const XpReaderContext& context);

  /* Add metadata from key/value pairs */
  void bindMetadata(const QStringList& line, const XpReaderContext& context);

  /* Add viewpoint as tower position */
  void bindViewpoint(const QStringList& line, const XpReaderContext& context);

  /* Add fuel flags from truck parking positions */
  void bindFuel(const QStringList& line, const XpReaderContext& context);

  /* Finalize and write airport */
  void finishAirport(const XpReaderContext& context);

  /* Collect runway data and write start positions */
  void bindRunway(const QStringList& line, AirportRowCode airportRowCode, const XpReaderContext& context);

  /* Write helipad and start positions */
  void writeHelipad(const QStringList& line, const XpReaderContext& context);

  /* TWR, ASOS, ATIS, etc. */
  void writeCom(const QStringList& line, AirportRowCode rowCode, const XpReaderContext& context, bool spacing833Khz);

  /* Add vasi to runway end */
  void bindVasi(const QStringList& line, const XpReaderContext& context);

  /* File metadata for lookup in GUI*/
  void writeAirportFile(const QString& icao, int curFileId);

  /* Start pavement (taxi and apron) by header */
  void bindPavement(const QStringList& line, const atools::fs::xp::XpReaderContext& context);
  void bindPavementNode(const QStringList& line, atools::fs::xp::AirportRowCode rowCode,
                        const XpReaderContext& context);
  void finishPavement(const XpReaderContext& context);

  /* Obsolete type 15 */
  void writeStartup(const QStringList& line, const XpReaderContext& context);

  /* Write parking */
  void writeStartupLocation(const QStringList& line, const XpReaderContext& context);
  void writeStartupLocationMetadata(const QStringList& line, const XpReaderContext& context);
  void finishStartupLocation();

  /* Collect taxi nodes (not written) */
  void bindTaxiNode(const QStringList& line, const XpReaderContext& context);

  /* Write taxi edges */
  void bindTaxiEdge(const QStringList& line, const XpReaderContext& context);

  int compareGate(const QString& gate1, const QString& gate2);
  int compareRamp(const QString& ramp1, const QString& ramp2);

  /* Calculate center of parking spot for position of tick mark */
  void calculateParkingPos(geo::Pos& position, float heading, float radiusFeet);

  float transitionAltOrLevel(const QString& str);

  /* State information */
  bool writingAirport = false, ignoringAirport = false,
       writingPavementBoundary = false, writingPavementHoles = false, writingPavementNewHole = false,
       writingStartLocation = false, airportClosed = false;

  /* Current feature ids */
  int curAirportId = 0, curRunwayEndId = 0, curHelipadId = 0, curComId = 0, curStartId = 0,
      curParkingId = 0, curApronId = 0, curTaxiPathId = 0, curHelipadStartNumber = 0,
      curAirportFileId = 10000000 /* Needs to count down since reading order is reversed */;

  bool hasTower = false, is3d = false;

  /* Counters for redundant airport data */
  int numRunwayEndAls = 0, numHardRunway = 0, numApron = 0,
      numSoftRunway = 0, numRunway = 0, numWaterRunway = 0, numLightRunway = 0, numHelipad = 0,
      numCom = 0, numStart = 0, numParking = 0, numTaxiPath = 0,
      numRunwayEndVasi = 0,
      numParkingGaRamp = 0, numParkingGate = 0, numParkingCargo = 0,
      numParkingMilCargo = 0, numParkingMilCombat = 0;

  atools::sql::SqlRecordList runwayEndRecords;
  /* pre-filled record */
  const atools::sql::SqlRecord runwayEndRecord;

  /* Keep runway information to ease assigning of VASI to a runway end */
  QVector<atools::fs::xp::RunwayGeometry> runwayGeometry;

  /* Keep runway ends until ICAO code is determined */
  QVector<atools::fs::xp::RunwayEnds> runwayEnds;

  /* Collect runways to determine longest ============================================================== */
  QVector<atools::fs::xp::RunwayDimension> runwayDimensions;

  float airportAltitude = 0.f;
  AirportRowCode airportRowCode = NO_ROWCODE;

  atools::sql::SqlQuery *insertAirportQuery = nullptr,
                        *insertRunwayQuery = nullptr, *insertRunwayEndQuery = nullptr, *insertHelipadQuery = nullptr,
                        *insertComQuery = nullptr, *insertApronQuery = nullptr, *insertTaxiQuery = nullptr,
                        *insertStartQuery = nullptr, *insertParkingQuery = nullptr, *insertAirportFileQuery = nullptr;

  QString largestParkingRamp, largestParkingGate;
  QString airportIdent, airportIcao, airportIata, airportFaa, airportLocal;

  atools::geo::Rect airportRect;
  atools::geo::Pos airportPos, airportDatumPos;
  atools::fs::common::AirportIndex *airportIndex;
  atools::fs::common::XpGeometry currentPavement;
  QHash<int, atools::geo::Pos> taxiNodes;

};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_AIRPORTREADER_h
