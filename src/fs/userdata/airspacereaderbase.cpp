/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include "fs/userdata/airspacereaderbase.h"

#include "geo/pos.h"
#include "geo/rect.h"
#include "geo/calculations.h"
#include "fs/util/coordinates.h"
#include "fs/common/binarygeometry.h"
#include "exception.h"
#include "sql/sqlutil.h"
#include "sql/sqlquery.h"

#include <QFile>
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
namespace userdata {

// Extract values from AH and AL values
static QRegularExpression MATCH_ALT("^(FL)?\\s*([0-9]+)\\s*(FL|FT|M[ $])?\\s*(AMSL|MSL|AGL|GND|AAGL|ASFC)?");

AirspaceReaderBase::AirspaceReaderBase(atools::sql::SqlDatabase *sqlDb)
  : db(sqlDb)
{
  initQueries();
}

void AirspaceReaderBase::readLine(const QStringList& line, int fileIdParam, const QString& filenameParam, int lineNumberParam)
{
  Q_UNUSED(line)
  Q_UNUSED(fileIdParam)
  Q_UNUSED(filenameParam)
  Q_UNUSED(lineNumberParam)

  // No op per default
}

void AirspaceReaderBase::finish()
{
  // No op per default
}

AirspaceReaderBase::~AirspaceReaderBase()
{
  deInitQueries();
}

void AirspaceReaderBase::reset()
{
  insertAirspaceQuery->clearBoundValues();
}

void AirspaceReaderBase::resetErrors()
{
  errors.clear();
}

void AirspaceReaderBase::resetNumRead()
{
  numAirspacesRead = 0;
}

AirspaceReaderBase::Format AirspaceReaderBase::detectFileFormat(const QString& file)
{
  QString probe = atools::probeFile(file).constFirst();

  // Check if file is IVAO JSON which starts with an array at top level
  if(probe.startsWith('['))
    return IVAO_JSON;

  // Check if file is a GeoJSON which starts with an object at top level
  if(probe.startsWith('{'))
    return VATSIM_GEO_JSON;

  // OpenAir starts with a comment "*" or an upper case letter
  if(atools::strStartsWith({"*", "ac", "an", "ah", "al"}, probe))
    return OPEN_AIR;

  return UNKNOWN;
}

void AirspaceReaderBase::initQueries()
{
  deInitQueries();

  insertAirspaceQuery = new SqlQuery(db);
  insertAirspaceQuery->prepare(SqlUtil(db).buildInsertStatement("boundary"));
}

void AirspaceReaderBase::deInitQueries()
{
  delete insertAirspaceQuery;
  insertAirspaceQuery = nullptr;
}

QString AirspaceReaderBase::mid(const QStringList& line, int index, bool ignoreError)
{
  if(index < line.size())
    return line.mid(index).join(" ");
  else if(!ignoreError)
    // Have to stop reading the file since the rest can be corrupted
    throw atools::Exception(tr("In file \"%1\" on line %2: Index out of bounds: Index: %1, size: %2").
                            arg(filename).arg(lineNumber).arg(index).arg(line.size()));
  return QString();
}

void AirspaceReaderBase::errWarn(const QString& msg)
{
  qWarning() << filename << ":" << lineNumber << msg;
  errors.append({filename, msg, lineNumber});
}

} // namespace userdata
} // namespace fs
} // namespace atools
