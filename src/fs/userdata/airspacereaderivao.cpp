/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "fs/userdata/airspacereaderivao.h"

#include "fs/util/fsutil.h"
#include "geo/calculations.h"
#include "fs/util/coordinates.h"
#include "fs/common/binarygeometry.h"
#include "exception.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::geo::Pos;
using atools::geo::Rect;
using atools::geo::LineString;

namespace atools {
namespace fs {
namespace userdata {

AirspaceReaderIvao::AirspaceReaderIvao(atools::sql::SqlDatabase *sqlDb)
  : AirspaceReaderBase(sqlDb)
{
}

AirspaceReaderIvao::~AirspaceReaderIvao()
{
}

// .[
// .  {
// .    "airport_id": "SUMU",
// .    "middle_identifier": null,
// .    "position": "APP",
// .    "map_region": [
// .      {
// .        "lat": -34.982778,
// .        "lng": -56.883889
// .      },
// .      {
// .........................
// .      {
// .        "lat": -34.983333,
// .        "lng": -56.883333
// .      }
// .    ],
// .    "type": "TMA",
// .    "name": "Carrasco Radar"
// .  },
// .........................
// .  {
// .    "airport_id": "KBNA",
// .    "middle_identifier": null,
// .    "position": "DEP",
// .    "map_region": [],
// .    "type": "TMA",
// .    "name": "Nashville Departure"
// .  },
// .........................
// .    ],
// .    "type": "FIR",
// .    "name": "Madrid Radar"
// .  }
// .]
//
// airport_id center_id lat lng map_region middle_identifier name position type
//
// .CREATE TABLE boundary (
// .    boundary_id             INTEGER       PRIMARY KEY,
// .    file_id                 INTEGER       NOT NULL,
// .    type                    VARCHAR (15),
// .    name                    VARCHAR (250),
// .    restrictive_designation VARCHAR (20),
// .    restrictive_type        VARCHAR (20),
// .    multiple_code           VARCHAR (5),
// .    time_code               VARCHAR (5),
// .    com_type                VARCHAR (30),
// .    com_frequency           INTEGER,
// .    com_name                VARCHAR (50),
// .    min_altitude_type       VARCHAR (15),
// .    max_altitude_type       VARCHAR (15),
// .    min_altitude            INTEGER,
// .    max_altitude            INTEGER,
// .    max_lonx                DOUBLE        NOT NULL,
// .    max_laty                DOUBLE        NOT NULL,
// .    min_lonx                DOUBLE        NOT NULL,
// .    min_laty                DOUBLE        NOT NULL,
// .    geometry                BLOB,
// .);
bool AirspaceReaderIvao::readFile(const QString& filenameParam)
{
  reset();
  resetErrors();
  resetNumRead();

  filename = filenameParam;

  QFile file(filename);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if(error.error == QJsonParseError::NoError)
    {
      QJsonArray arr = doc.array();
      for(int i = 0; i < arr.count(); i++)
      {
        QJsonObject obj = arr.at(i).toObject();

        // "position": "APP", -> A
        // "position": "ATIS", -> null
        // "position": "CTR", -> C
        // "position": "DEL", -> CL
        // "position": "DEP", -> D
        // "position": "FSS", -> FSS
        // "position": "GND", -> G
        // "position": "TWR", -> T
        QString position = obj.value("position").toString();

        // "type": "FIR",
        // "type": "TMA",
        // QString type = obj.value("type").toString();

        QString airportId = obj.value("airport_id").toString();
        QString middleIdent = obj.value("middle_identifier").toString();
        QString name = obj.value("name").toString();

        // Fetch geometry ==================================================
        LineString line;
        QJsonArray coordinates = obj.value("map_region").toArray();
        for(int coordIdx = 0; coordIdx < coordinates.count(); coordIdx++)
        {
          QJsonObject coordObj = coordinates.at(coordIdx).toObject();
          line.append(Pos(coordObj.value("lng").toDouble(), coordObj.value("lat").toDouble()));
        }

        QString dbType = positionToDbType(position);

        if(!dbType.isEmpty() && !airportId.isEmpty())
        {
          insertAirspaceQuery->bindValue(":boundary_id", airspaceId++);
          insertAirspaceQuery->bindValue(":file_id", fileId);
          insertAirspaceQuery->bindValue(":description", name);
          insertAirspaceQuery->bindValue(":type", dbType);

          // Unused fields here
          insertAirspaceQuery->bindNullStr(":restrictive_designation");
          insertAirspaceQuery->bindNullStr(":restrictive_type");
          insertAirspaceQuery->bindNullStr(":multiple_code");
          insertAirspaceQuery->bindValue(":time_code", "U");

          // Build ident string like "LEPA_S_TWR" as used in IVAO data JSON ==================
          QStringList ident({airportId, middleIdent, position});
          ident.removeAll(QString());
          insertAirspaceQuery->bindValue(":name", ident.join('_'));

          // Get airport position from callback ====================
          Pos airportPos;
          if(fetchAirportCoords)
            airportPos = fetchAirportCoords(airportId);

          Rect bounding = line.boundingRect();

          if(airportPos.isValid() && (line.size() < 3 || bounding.isPoint() || !bounding.isValid()))
          {
            // Replace invalid geometry with airport position
            line.clear();
            line.append(airportPos);
            bounding = line.boundingRect();
          }

          line.removeInvalid();
          bounding = line.boundingRect();

          if(!line.isEmpty())
          {
            insertAirspaceQuery->bindValue(":max_lonx", bounding.getEast());
            insertAirspaceQuery->bindValue(":max_laty", bounding.getNorth());
            insertAirspaceQuery->bindValue(":min_lonx", bounding.getWest());
            insertAirspaceQuery->bindValue(":min_laty", bounding.getSouth());

            // Create geometry blob
            atools::fs::common::BinaryGeometry geo(atools::fs::util::correctBoundary(line));
            insertAirspaceQuery->bindValue(":geometry", geo.writeToByteArray());

            insertAirspaceQuery->exec();
            insertAirspaceQuery->clearBoundValues();
            numAirspacesRead++;
          }
          else
            qWarning() << Q_FUNC_INFO << "No geometry found" << airportId << middleIdent << position;
        }
        else
          qWarning() << Q_FUNC_INFO << "Invalid type" << airportId << middleIdent << position;

        reset();
      } // for(int i = 0; i < arr.count(); i++)
    } // if(error.error == QJsonParseError::NoError)
    else
      qWarning() << Q_FUNC_INFO << "Error reading" << filename << error.errorString() << "at offset" << error.offset;

    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2 (%3)").arg(filename).arg(file.errorString()).arg(file.error()));

  return numAirspacesRead > 0;
}

QString AirspaceReaderIvao::positionToDbType(const QString& position)
{
  if(position == "APP")
    return "A";

  if(position == "ATIS")
    return QString();

  if(position == "CTR")
    return "C";

  if(position == "DEL")
    return "CL";

  if(position == "DEP")
    return "D";

  if(position == "FSS")
    return "FSS";

  if(position == "GND")
    return "G";

  if(position == "TWR")
    return "T";

  return QString();
}

} // namespace userdata
} // namespace fs
} // namespace atools
