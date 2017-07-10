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

#include <QApplication>

namespace atools {

namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {
namespace xp {

class XpAirportWriter :
  public atools::fs::xp::XpWriter
{
  Q_DECLARE_TR_FUNCTIONS(XpAirportWriter)

public:
  XpAirportWriter(atools::sql::SqlDatabase& sqlDb);
  virtual ~XpAirportWriter();

  virtual void write(const QStringList& line, int curFileId) override;
  virtual void finish() override;

private:
  void initQueries();
  void deInitQueries();

  void bindAirport(const QStringList& line, atools::fs::xp::AirportRowCode airportRowCode, int curFileId);
  void finishAirport();
  void bindRunway(const QStringList& line, AirportRowCode airportRowCode);
  void bindMetadata(const QStringList& line);
  bool hasAirport(const QString& ident);
  void bindHelipad(const QStringList& line);
  void bindCom(const QStringList& line, AirportRowCode rowCode);
  void bindViewpoint(const QStringList& line);
  void bindStart(const QStringList& line);
  void bindStartupLocation(const QStringList& line);
  void bindPattern(const QStringList& line);

  bool writingAirport = false, ignoringAirport = false;

  int curAirportId = 0, curRunwayId = 0, curRunwayEndId = 0, curHelipadId = 0, curComId = 0, curStartId = 0;

  int numRunwayEndAls = 0, numRunwayEndIls = 0, numHardRunway = 0,
      numRunwayEndClosed = 0, numSoftRunway = 0, numWaterRunway = 0, numLightRunway = 0, numHelipad = 0,
      numCom = 0, numStart = 0,
      numRunwayEndVasi = 0, numJetway = 0, numBoundaryFence = 0,
      numParkingGaRamp = 0, numParkingGate = 0, numParkingCargo = 0, numParkingMilitaryCargo = 0,
      numParkingMilitaryCombat = 0;

  float airportAltitude = 0.f;
  float longestRunwayLength = 0.f, longestRunwayWidth = 0.f, longestRunwayHeading = 0.f;

  QString longestRunwaySurface = "UNKNOWN";
  // atools::fs::bgl::ap::ParkingType largestParkingGaRamp = atools::fs::bgl::ap::UNKNOWN,
  // largestParkingGate = atools::fs::bgl::ap::UNKNOWN;
  // bool towerObj = false, airportClosed = false, military = false;

  AirportRowCode airportRowCode = NO_ROWCODE;

  atools::sql::SqlQuery *insertAirportQuery = nullptr, *selectAirportQuery = nullptr,
                        *insertRunwayQuery = nullptr, *insertRunwayEndQuery = nullptr, *insertHelipadQuery = nullptr,
                        *insertComQuery = nullptr, *insertStartQuery = nullptr;

  atools::geo::Rect bounding;
  atools::geo::Pos airportPos;
};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_AIRPORTWRITER_H
