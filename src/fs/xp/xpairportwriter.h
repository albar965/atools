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

class NavDatabaseOptions;
class ProgressHandler;

namespace xp {

class XpAirportIndex;

class XpAirportWriter :
  public atools::fs::xp::XpWriter
{
  Q_DECLARE_TR_FUNCTIONS(XpAirportWriter)

public:
  XpAirportWriter(atools::sql::SqlDatabase& sqlDb, atools::fs::xp::XpAirportIndex *xpAirportIndex,
                  const atools::fs::NavDatabaseOptions& opts, atools::fs::ProgressHandler *progressHandler);
  virtual ~XpAirportWriter();

  virtual void write(const QStringList& line, const XpWriterContext& context) override;
  virtual void finish(const XpWriterContext& context) override;

private:
  void initQueries();
  void deInitQueries();

  void bindAirport(const QStringList& line, atools::fs::xp::AirportRowCode airportRowCode,
                   const XpWriterContext& context);
  void bindMetadata(const QStringList& line);
  void bindViewpoint(const QStringList& line);
  void bindFuel(const QStringList& line);
  void finishAirport(const XpWriterContext& context);

  void writeRunway(const QStringList& line, AirportRowCode airportRowCode);
  void writeHelipad(const QStringList& line);
  void writeCom(const QStringList& line, AirportRowCode rowCode);
  void writeAirportFile(const QString& icao, int curFileId);
  void bindVasi(const QStringList& line);

  void bindPavement(const QStringList& line);
  void bindPavementNode(const QStringList& line, atools::fs::xp::AirportRowCode rowCode);
  void finishPavement();

  void writeParking(const QStringList& line);

  void writeStartupLocation(const QStringList& line);
  void writeStartupLocationMetadata(const QStringList& line);
  void finishStartupLocation();

  void bindTaxiNode(const QStringList& line);
  void bindTaxiEdge(const QStringList& line);

  bool writingAirport = false, ignoringAirport = false,
       writingPavementBoundary = false, writingPavementHoles = false, writingPavementNewHole = false,
       writingStartLocation = false;

  int curAirportId = 0, curRunwayId = 0, curRunwayEndId = 0, curHelipadId = 0, curComId = 0, curStartId = 0,
      curParkingId = 0, curApronId = 0, curTaxiPathId = 0, curHelipadStartNumber = 0,
      curAirportFileId = 10000000 /* Needs to count down since reading order is reversed */;

  bool hasTower = false;

  int numRunwayEndAls = 0, numRunwayEndIls = 0, numHardRunway = 0, numApron = 0,
      numRunwayEndClosed = 0, numSoftRunway = 0, numWaterRunway = 0, numLightRunway = 0, numHelipad = 0,
      numCom = 0, numStart = 0, numParking = 0, numTaxiPath = 0,
      numRunwayEndVasi = 0, numJetway = 0, numBoundaryFence = 0,
      numParkingGaRamp = 0, numParkingGate = 0, numParkingCargo = 0,
      numParkingMilitaryCombat = 0, numParkingMilCargo = 0, numParkingMilCombat = 0;

  atools::sql::SqlRecordVector runwayEndRecords;
  const atools::sql::SqlRecord runwayEndRecord;

  float airportAltitude = 0.f;
  float longestRunwayLength = 0.f, longestRunwayWidth = 0.f, longestRunwayHeading = 0.f;

  QString longestRunwaySurface = "UNKNOWN";
  // atools::fs::bgl::ap::ParkingType largestParkingGaRamp = atools::fs::bgl::ap::UNKNOWN,
  // largestParkingGate = atools::fs::bgl::ap::UNKNOWN;
  // bool towerObj = false, airportClosed = false, military = false;

  AirportRowCode airportRowCode = NO_ROWCODE;

  atools::sql::SqlQuery *insertAirportQuery = nullptr,
                        *insertRunwayQuery = nullptr, *insertRunwayEndQuery = nullptr, *insertHelipadQuery = nullptr,
                        *insertComQuery = nullptr, *insertApronQuery = nullptr, *insertTaxiQuery = nullptr,
                        *insertStartQuery = nullptr, *insertParkingQuery = nullptr, *insertAirportFileQuery = nullptr;

  QString largestParkingRamp, largestParkingGate;
  QString airportIcao;

  atools::geo::Rect airportRect;
  atools::geo::Pos airportPos;
  atools::fs::xp::XpAirportIndex *airportIndex;
  atools::fs::common::XpGeometry currentPavement;
  QHash<int, atools::geo::Pos> taxiNodes;
};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_AIRPORTWRITER_H
