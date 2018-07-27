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

#include "fs/xp/xpairspacewriter.h"

#include "fs/common/airportindex.h"
#include "fs/xp/xpconstants.h"
#include "fs/progresshandler.h"
#include "fs/common/magdecreader.h"
#include "geo/pos.h"
#include "geo/rect.h"
#include "geo/calculations.h"
#include "fs/util/coordinates.h"
#include "fs/common/binarygeometry.h"

#include "sql/sqlutil.h"

#include <QRegularExpression>
#include <QTextCodec>

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::geo::Pos;
using atools::geo::Rect;
using atools::geo::LineString;
using atools::fs::util::fromOpenAirFormat;

namespace atools {
namespace fs {
namespace xp {

// Extract values from AH and AL values
static QRegularExpression MATCH_ALT("^(FL)?\\s*([0-9]+)\\s*(FL|FT|M[ $])?\\s*(AMSL|MSL|AGL|GND|AAGL|ASFC)?");

XpAirspaceWriter::XpAirspaceWriter(atools::sql::SqlDatabase& sqlDb,
                                   const NavDatabaseOptions& opts, ProgressHandler *progressHandler,
                                   atools::fs::NavDatabaseErrors *navdatabaseErrors)
  : XpWriter(sqlDb, opts, progressHandler, navdatabaseErrors)
{
  initQueries();
}

XpAirspaceWriter::~XpAirspaceWriter()
{
  deInitQueries();
}

void XpAirspaceWriter::write(const QStringList& line, const XpWriterContext& context)
{
  ctx = &context;

  QString key = at(line, 0);

  if(writingCoordinates && !key.startsWith("D") && key != "V")
  {
    // Done with coordinates - write old boundary an start a new one
    writingCoordinates = false;
    writeBoundary();
  }

  if(key.startsWith("D") || key == "V")
  {
    // Geomety or variables
    writingCoordinates = true;
    bindCoordinate(line);
  }
  else if(key == "AC")
    // Class
    bindClass(at(line, 1));
  else if(key == "AN")
    // Name
    bindName(mid(line, 1));
  else if(key == "AH")
    // Upper limit
    bindAltitude(line, true /* max altitude */);
  else if(key == "AL")
    // Lower limit
    bindAltitude(line, false /* min altitude */);
}

void XpAirspaceWriter::writeBoundary()
{
  // insertAirspaceQuery->bindValue(":com_type", );
  // insertAirspaceQuery->bindValue(":com_frequency", );
  // insertAirspaceQuery->bindValue(":com_name", );
  // insertAirspaceQuery->bindValue(":comment", );

  insertAirspaceQuery->bindValue(":boundary_id", ++curAirspaceId);
  insertAirspaceQuery->bindValue(":file_id", ctx->curFileId);

  // Remove all remaining invalid points
  LineString::iterator it = std::remove_if(curLine.begin(), curLine.end(), [](const Pos& p) -> bool
        {
          return !p.isValidRange();
        });

  if(it != curLine.end())
  {
    errWarn("Found invalid coordinates in airspace");
    curLine.erase(it, curLine.end());
  }

  if(curLine.size() > 2)
  {
    // calculate bounding rectangle
    Rect bounding = curLine.boundingRect();

    if(!bounding.isPoint())
    {
      insertAirspaceQuery->bindValue(":max_lonx", bounding.getEast());
      insertAirspaceQuery->bindValue(":max_laty", bounding.getNorth());
      insertAirspaceQuery->bindValue(":min_lonx", bounding.getWest());
      insertAirspaceQuery->bindValue(":min_laty", bounding.getSouth());

      // Create geometry blob
      atools::fs::common::BinaryGeometry geo(curLine);
      insertAirspaceQuery->bindValue(":geometry", geo.writeToByteArray());

      // Fields not used by X-Plane
      insertAirspaceQuery->bindValue(":multiple_code", QVariant(QVariant::String));
      insertAirspaceQuery->bindValue(":time_code", "U");

      insertAirspaceQuery->exec();

      progress->incNumBoundaries();
    }
    else
      errWarn("Found invalid bounding rectangle for airspace");
  }
  else
    errWarn("Airspace has not enough points");
  reset();
}

void XpAirspaceWriter::bindCoordinate(const QStringList& line)
{
  QString key = at(line, 0).toUpper();
  QString value = mid(line, 1).trimmed().toUpper();

  if(key == "DP")
  {
    // DP coordinate - add polygon point
    Pos pos = fromOpenAirFormat(value);
    if(pos.isValidRange())
      curLine.append(pos);
    else
      errWarn("Found invalid coordinates in airspace record DP: \"" + value + "\"");
  }
  else if(key == "DA")
  {
    // DA radius, angleStart, angleEnd - add an arc, angles in degrees, radius in nm (set center using V X=...)
    float radius = value.section(',', 0, 0).trimmed().toFloat();
    float angleStart = value.section(',', 1, 1).trimmed().toFloat();
    float angleEnd = value.section(',', 2, 2).trimmed().toFloat();

    // Check minimum radius since some use it for color definitions
    if(radius > 0.2f)
    {
      Pos pos1 = center.endpoint(atools::geo::nmToMeter(radius), angleStart);
      Pos pos2 = center.endpoint(atools::geo::nmToMeter(radius), angleEnd);
      if(pos1.isValid() && pos2.isValid() && center.isValidRange())
        curLine.append(LineString(center, pos1, pos2, clockwise, 24));
      else
        errWarn("Found invalid coordinates in airspace record DA: \"" + value + "\"");
    }
    clockwise = true;
  }
  else if(key == "DB")
  {
    // DB coordinate1, coordinate2 - add an arc, from coordinate1 to coordinate2 (set center using V X=...)
    Pos pos1 = fromOpenAirFormat(value.section(',', 0, 0).trimmed());
    Pos pos2 = fromOpenAirFormat(value.section(',', 1, 1).trimmed());

    if(pos1.isValid() && pos2.isValid() && center.isValidRange())
      curLine.append(LineString(center, pos1, pos2, clockwise, 24));
    else
      errWarn("Found invalid coordinates in airspace record DB: \"" + value + "\"");
    clockwise = true;
  }
  else if(key == "DC")
  {
    // DC radius - draw a circle (center taken from the previous V X=... record, radius in nm
    float radius = value.toFloat();
    if(radius > 0.2f && center.isValidRange())
      curLine.append(LineString(center, atools::geo::nmToMeter(radius), 24));
    else
      // Small values are apparently used to define colors
      qWarning() << ctx->messagePrefix() << "Found invalid radius or center coordinate in airspace record DC" << value;
    clockwise = true;
  }
  else if(key == "V")
  {
    // V x=n - Variable assignment. Currently the following variables are supported:
    QString variableName = value.section('=', 0, 0).trimmed();
    QString variableValue = value.section('=', 1).trimmed();

    if(variableName == "D")
    {
      if(variableValue == "+" || variableValue == "-")
        // D={+|-} sets direction for: DA and DB records
        // '-' means counterclockwise direction; '+' is the default
        // automatically reset to '+' at the begining of new airspace segment
        clockwise = variableValue == "+";
      else
        errWarn("Invalid direction value in airspace record D: \"" + variableValue + "\"");
    }
    else if(variableName == "X")
    {
      // X=coordinate : sets the center for the following records: DA, DB, and DC
      center = fromOpenAirFormat(variableValue);
      if(!center.isValidRange())
        errWarn("Invalid center coordinate in airspace record D: \"" + variableValue + "\"");
    }
  }
}

void XpAirspaceWriter::bindName(const QString& name)
{
  insertAirspaceQuery->bindValue(":name", name);
}

void XpAirspaceWriter::bindClass(const QString& cls)
{
  QString type;

  if(cls == "R") // restricted
    type = "R";
  else if(cls == "Q") // danger
    type = "DA";
  else if(cls == "P") // prohibited
    type = "P";
  else if(cls == "A") // Class A
    type = "CA";
  else if(cls == "B") // Class B
    type = "CB";
  else if(cls == "C") // Class C
    type = "CC";
  else if(cls == "D") // Class D
    type = "CD";
  else if(cls == "E") // Class E
    type = "CE";
  else if(cls == "F") // Class F
    type = "CE";
  else if(cls == "G") // Class G
    type = "CE";
  else if(cls == "GP") // glider prohibited
    type = "GP";
  else if(cls == "CTR") // CTR
    type = "C";
  else if(cls == "W") // Wave Window
    type = "WW";
  else if(cls == "RMZ") // Radio Mandatory Zone
    type = "CG"; // Class G
  else if(cls == "TMZ") // Transponder Mandatory Zone
    type = "MC"; // Mode C
  else
    qWarning() << ctx->messagePrefix() << "Unknown airspace class" << cls;
  insertAirspaceQuery->bindValue(":type", type);
}

void XpAirspaceWriter::bindAltitude(const QStringList& line, bool isMax)
{
  QString prefix = isMax ? ":max" : ":min";
  int unlimited = isMax ? 100000 : 0;
  int altitude = unlimited;

  QString altStr, type;
  if(line.size() > 1)
    // Leave unknown if not given
    altStr = mid(line, 1).toUpper();

  if(altStr.startsWith("UN"))
  {
    // Unlimited keyword no number
    if(isMax)
      type = "UL";
    else
    {
      // Not for lower boundary - set to ground
      type = "AGL";
      altitude = 0;
    }
  }
  else if(altStr.startsWith("SFC") || altStr.startsWith("GND") || altStr.startsWith("MSL"))
  {
    // No number - only keyword
    if(isMax)
      // Set to unlimited for upper limit on ground
      type = "UL";
    else
    {
      type = "AGL";
      altitude = 0;
    }
  }
  else
  {
    // Number and several keywords match
    QRegularExpressionMatch match = MATCH_ALT.match(altStr);
    if(match.hasMatch())
    {
      altitude = match.captured(2).toInt();
      if(match.captured(1) == "FL")
      {
        // Flight level as prefix no keyword needed
        type = "MSL";
        altitude *= 100;
      }
      else
      {
        if(match.captured(3) == "FL")
        {
          // Flight level as suffix ignore the rest
          altitude *= 100;
          type = "MSL";
        }
        else
        {
          if(match.captured(3) == "M")
            // Meter suffix
            altitude = atools::geo::meterToFeet(altitude);

          // Ground level keyword
          QString t = match.captured(4);
          type = "MSL";
          if(t == "AGL" || t == "GND" || t == "AAGL" || t == "ASFC" || t == "SFC" || t == "GND")
            type = "AGL";
          // else if(type == "AMSL" || type == "MSL")
          // altType = "MSL";
        }
      }
    }
  }

  insertAirspaceQuery->bindValue(prefix + "_altitude_type", type);
  insertAirspaceQuery->bindValue(prefix + "_altitude", altitude);
}

void XpAirspaceWriter::finish(const XpWriterContext& context)
{
  ctx = &context;
  writeBoundary();
}

void XpAirspaceWriter::reset()
{
  insertAirspaceQuery->clearBoundValues();
  curLine.clear();
  clockwise = true;
  writingCoordinates = false;
  center = Pos();
}

void XpAirspaceWriter::initQueries()
{
  deInitQueries();

  SqlUtil util(&db);

  insertAirspaceQuery = new SqlQuery(db);
  insertAirspaceQuery->prepare(util.buildInsertStatement("boundary"));
}

void XpAirspaceWriter::deInitQueries()
{
  delete insertAirspaceQuery;
  insertAirspaceQuery = nullptr;
}

} // namespace xp
} // namespace fs
} // namespace atools
