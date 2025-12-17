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

#include "fs/xp/xpcifpreader.h"

#include "fs/common/airportindex.h"
#include "fs/common/procedurewriter.h"
 #include "atools.h"

namespace atools {
namespace fs {
namespace xp {

/* *INDENT-OFF* */
enum ProcedureFieldIndex
{                               // Index in ARINC file / chapter in documentation
  PROC_ROW_CODE           =  0, // All: 4.1.9.1. Page 55 (70)
  SEQ_NR                  =  1, // 27-29   5.12
  RT_TYPE                 =  2, // 20      5.7
  SID_STAR_APP_IDENT      =  3, // 14-19   5.9&5.10 Examples: DEPU2, SCK4, TRP7, 41M3, MONTH6
  TRANS_IDENT             =  4, // 21-25   5.11
  FIX_IDENT               =  5, // 30-34   5.13
  ICAO_CODE               =  6, // 35-36   5.14
  SEC_CODE                =  7, // 37      5.4
  SUB_CODE                =  8, // 38      5.5
  DESC_CODE               =  9, // 40-43   5.17
  TURN_DIR                = 10, // 44      5.20
  RNP                     = 11, // 45-47   5.211
  PATH_TERM               = 12, // 48-49   5.21
  TDV                     = 13, // 50      5.22
  RECD_NAVAID             = 14, // 51-54   5.23
  RECD_ICAO_CODE          = 15, // 55-56   5.14
  RECD_SEC_CODE           = 16, // 79      5.4
  RECD_SUB_CODE           = 17, // 80      5.5
  ARC_RADIUS              = 18, // 57-62   5.204
  THETA                   = 19, // 63-66   5.24
  RHO                     = 20, // 67-70   5.25
  MAG_CRS                 = 21, // 71-74   5.26
  RTE_DIST_HOLD_DIST_TIME = 22, // 75-78   5.27
  ALT_DESCR               = 23, // 83      5.29
  ALTITUDE                = 24, // 85-89   5.30
  ALTITUDE2               = 25, // 90-94   5.30
  TRANS_ALT               = 26, // 95-99   5.53
  SPD_LIMIT_DESCR         = 27, // 118     5.261
  SPEED_LIMIT             = 28, // 100-102 5.72
  VERT_ANGLE              = 29, // 103-106 5.70
  UNKNOWN_INDEX           = 30, // 121-123 5.293 not in
  CENTER_FIX_OR_TAA_PT    = 31, // 107-111 5.144/5.271
  CENTER_ICAO_CODE        = 32, // 113-114 5.14
  CENTER_SEC_CODE         = 33, // 115     5.4
  CENTER_SUB_CODE         = 34, // 116     5.5
  MULTI_CD                = 35, // 112     5.130/5.272
  GNSS_FMS_IND            = 36, // 117     5.222
  RTE_QUAL1               = 37, // 119     5.7
  RTE_QUAL2               = 38  // 120     5.7
};
/* *INDENT-ON* */

XpCifpReader::XpCifpReader(atools::sql::SqlDatabase& sqlDb, atools::fs::common::AirportIndex *airportIndexParam,
                           const NavDatabaseOptions& opts, ProgressHandler *progressHandler,
                           atools::fs::NavDatabaseErrors *navdatabaseErrors)
  : XpReader(sqlDb, opts, progressHandler, navdatabaseErrors)
{
  procWriter = new atools::fs::common::ProcedureWriter(sqlDb, airportIndexParam);
}

XpCifpReader::~XpCifpReader()
{
  delete procWriter;
}

void XpCifpReader::read(const QStringList& line, const XpReaderContext& context)
{
  ctx = &context;
  if(line.isEmpty())
    return;

  QString rowCode = line.at(PROC_ROW_CODE);
  if(!(rowCode == "SID" || rowCode == "STAR" || rowCode == "APPCH"))
    // Skip all unknown row codes
    return;

  atools::fs::common::ProcedureInput procInput;

  procInput.context = context.messagePrefix();
  procInput.airportIdent = context.cifpAirportIdent;
  procInput.airportId = context.cifpAirportId;

  procInput.rowCode = at(line, PROC_ROW_CODE).trimmed();
  procInput.seqNr = at(line, SEQ_NR).toInt();
  procInput.routeType = atools::strToChar(at(line, RT_TYPE));
  procInput.sidStarAppIdent = at(line, SID_STAR_APP_IDENT).trimmed();
  procInput.transIdent = at(line, TRANS_IDENT).trimmed();
  procInput.fixIdent = at(line, FIX_IDENT).trimmed();
  procInput.region = at(line, ICAO_CODE).trimmed();
  procInput.secCode = at(line, SEC_CODE);
  procInput.subCode = at(line, SUB_CODE);
  procInput.descCode = at(line, DESC_CODE);
  // procInput.aircraftCategory
  procInput.turnDir = at(line, TURN_DIR).trimmed();
  procInput.pathTerm = at(line, PATH_TERM).trimmed();
  procInput.recdNavaid = at(line, RECD_NAVAID).trimmed();
  procInput.recdRegion = at(line, RECD_ICAO_CODE).trimmed();
  procInput.recdSecCode = at(line, RECD_SEC_CODE);
  procInput.recdSubCode = at(line, RECD_SUB_CODE);

  procInput.theta = at(line, THETA).simplified().isEmpty() ? atools::fs::common::INVALID_FLOAT : at(line, THETA).toFloat() / 10.f;
  procInput.rho = at(line, RHO).simplified().isEmpty() ? atools::fs::common::INVALID_FLOAT : at(line, RHO).toFloat() / 10.f;
  procInput.magCourse = at(line, MAG_CRS).toFloat() / 10.f;

  QString rnpStr = at(line, RNP).simplified();
  if(rnpStr.isEmpty())
    procInput.rnp = atools::fs::common::INVALID_FLOAT;
  else
  {
    // RNP values are entered into the field in nautical miles (two digits) with a zero or negative exponent (one digit).
    // Examples: 990 (equal to 99.0NM), 120 (equal to 12.0NM), 013 (equal to 0.001NM)

    procInput.rnp = atools::fs::common::INVALID_FLOAT;
    char expChar = atools::latin1CharAt(rnpStr, 2);
    if(expChar == '0')
      procInput.rnp = rnpStr.toFloat() / 10.f;
    else
      procInput.rnp = rnpStr.left(2).toFloat() * std::pow(10.f, -static_cast<float>(expChar - '0'));
  }

  procInput.rteHoldTime = procInput.rteHoldDist = 0.f;
  QString distTime = at(line, RTE_DIST_HOLD_DIST_TIME).trimmed();
  if(distTime.startsWith("T"))
    // time minutes/10
    procInput.rteHoldTime = distTime.mid(1).toFloat() / 10.f;
  else
    // distance nm/10
    procInput.rteHoldDist = distTime.toFloat() / 10.f;

  procInput.altDescr = at(line, ALT_DESCR).trimmed();
  procInput.altitude = at(line, ALTITUDE).trimmed();
  procInput.altitude2 = at(line, ALTITUDE2).trimmed();
  procInput.transAlt = at(line, TRANS_ALT).trimmed();
  procInput.speedLimitDescr = at(line, SPD_LIMIT_DESCR).trimmed();
  procInput.speedLimit = at(line, SPEED_LIMIT).toInt();
  procInput.verticalAngle =
    at(line, VERT_ANGLE).simplified().isEmpty() ? QVariant(QVariant::Double) : at(line, VERT_ANGLE).toDouble() / 100.;
  procInput.centerFixOrTaaPt = at(line, CENTER_FIX_OR_TAA_PT).trimmed();
  procInput.centerIcaoCode = at(line, CENTER_ICAO_CODE).trimmed();
  procInput.centerSecCode = at(line, CENTER_SEC_CODE);
  procInput.centerSubCode = at(line, CENTER_SUB_CODE);
  procInput.gnssFmsIndicator = at(line, GNSS_FMS_IND);

  procWriter->write(procInput);
}

void XpCifpReader::finish(const XpReaderContext& context)
{
  atools::fs::common::ProcedureInput procInput;

  procInput.context = context.messagePrefix();
  procInput.airportIdent = context.cifpAirportIdent;
  procInput.airportId = context.cifpAirportId;

  procWriter->finish(procInput);
}

void XpCifpReader::reset()
{
  procWriter->reset();
}

} // namespace xp
} // namespace fs
} // namespace atools
