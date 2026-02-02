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

#include "fs/userdata/airspacereadervatsim.h"

#include "fs/common/binarygeometry.h"
#include "exception.h"
#include "fs/util/fsutil.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringBuilder>
#include <QRegularExpression>

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::geo::Pos;
using atools::geo::Rect;
using atools::geo::LineString;

namespace atools {
namespace fs {
namespace userdata {

AirspaceReaderVatsim::AirspaceReaderVatsim(atools::sql::SqlDatabase *sqlDb)
  : AirspaceReaderBase(sqlDb)
{
}

AirspaceReaderVatsim::~AirspaceReaderVatsim()
{
}

// .{
// .  "type": "FeatureCollection",
// .  "name": "VATSIM Map",
// .  "crs": {
// .    "type": "name",
// .    "properties": {
// .      "name": "urn:ogc:def:crs:OGC:1.3:CRS84"
// .    }
// .  },
// .  "features": [
// .    {
// .      "type": "Feature",
// .      "properties": {
// .        "id": "ADR",
// .        "oceanic": "0",
// .        "label_lon": "16.3",
// .        "label_lat": "42.9",
// .        "region": "EMEA",
// .        "division": "VATEUD"
// .      },
// .      "geometry": {
// .        "type": "MultiPolygon",
// .        "coordinates": [
// .        "coordinates": [
// .        "coordinates": [
// .          [
// .            [
// .              [
// .                18.866667,
// .                41.133333
// .              ],
// .........................
// .              [
// .                18.866667,
// .                41.133333
// .              ]
// .            ]
// .          ]
// .        ]
// .      }
// .    },
// .    {
// .      "type": "Feature",
// .      "properties": {
// .        "id": "ADR-E",
// .        "oceanic": "0",
// .        "label_lon": "20.8",
// .        "label_lat": "42.55",
// .        "region": "EMEA",
// .        "division": "VATEUD"
// .      },
// .      "geometry": {
// .        "type": "MultiPolygon",
// .        "coordinates": [
// .          [
// .            [
// .              [
// .                18.866667,
// .                41.133333
// .              ],
// .........................
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
bool AirspaceReaderVatsim::readFile(const QString& filenameParam)
{
  const static QRegularExpression REGEXP_DESCR(QStringLiteral("[\\-_]"));

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
      const QJsonArray featureArray = doc.object().value(QStringLiteral("features")).toArray();
      for(const QJsonValue& featureValue : featureArray)
      {
        QJsonObject featureObj = featureValue.toObject();
        QJsonObject propertiesObj = featureObj.value(QStringLiteral("properties")).toObject();
        QJsonObject geometryObj = featureObj.value(QStringLiteral("geometry")).toObject();

        // Used for name
        QString id = propertiesObj.value(QStringLiteral("id")).toString();

        // Region and division only for logging
        QString region = propertiesObj.value(QStringLiteral("region")).toString(); // only firboundaries.json
        QString division = propertiesObj.value(QStringLiteral("division")).toString(); //// only firboundaries.json

        // Collect prefixes from array
        QStringList prefixes;
        const QJsonArray prefixArray = propertiesObj.value(QStringLiteral("prefix")).toArray(QJsonArray());
        for(const QJsonValue& prefix :  prefixArray) // only traconboundaries.json
          prefixes.append(prefix.toString());

        // Skip unknown polygon types =====
        QString geometryType = geometryObj.value(QStringLiteral("type")).toString();
        if(geometryType != QStringLiteral("MultiPolygon"))
        {
          qWarning() << Q_FUNC_INFO << "Unexpected polygon type" << geometryType
                     << "Only MultiPolygon allowed" << "id" << id << "region" << region << "division" << division;
          continue;
        }

        // Read array of polygons ==============
        const QJsonArray geoArray = geometryObj.value(QStringLiteral("coordinates")).toArray();
        for(const QJsonValue& geometryValue : geoArray)
        {
          // Read array of polygon rings - first is outer ==============
          QJsonArray ringArr = geometryValue.toArray();
          for(int ringIndex = 0; ringIndex < ringArr.count(); ringIndex++)
          {
            if(ringIndex > 0)
            {
              // Ignore other rings - these are holes
              qWarning() << Q_FUNC_INFO << "Polygon has" << ringArr.count() << "rings"
                         << "Ignoring all except first." << "id" << id << "region" << region << "division" << division;
              break;
            }

            // Read coordinate pairs of a ring ===================
            LineString line;
            const QJsonArray ringArrays = ringArr.at(ringIndex).toArray();
            for(const QJsonValue& ringValue : ringArrays)
            {
              QJsonArray ringArray = ringValue.toArray();
              if(ringArray.size() == 2)
                line.append(atools::geo::Pos(ringArray.at(0).toDouble(), ringArray.at(1).toDouble()));
              else
                qWarning() << Q_FUNC_INFO << "Invalid number of ordinates" << ringArray.size()
                           << "id" << id << "region" << region << "division" << division;
            }

            line.removeInvalid();

            if(!line.isEmpty())
            {
              insertAirspaceQuery->bindValue(QStringLiteral(":boundary_id"), airspaceId++);
              insertAirspaceQuery->bindValue(QStringLiteral(":file_id"), fileId);

              // Unused fields here
              insertAirspaceQuery->bindNullStr(QStringLiteral(":restrictive_designation"));
              insertAirspaceQuery->bindNullStr(QStringLiteral(":restrictive_type"));
              insertAirspaceQuery->bindNullStr(QStringLiteral(":multiple_code"));
              insertAirspaceQuery->bindValue(QStringLiteral(":time_code"), QStringLiteral("U"));

              // Build boundary name
              QStringList ident;
              if(propertiesObj.contains(QStringLiteral("oceanic")))
              {
                // Center - only firboundaries.json
                ident.append(id.replace('-', '_').replace(QStringLiteral("__"), QStringLiteral("_")) % QStringLiteral("_CTR"));
                ident.removeAll(QString());
                insertAirspaceQuery->bindValue(QStringLiteral(":type"), QStringLiteral("C"));
                insertAirspaceQuery->bindValue(QStringLiteral(":name"), ident.join('_'));
                insertAirspaceQuery->bindValue(QStringLiteral(":description"),
                                               QString(id.section(REGEXP_DESCR, 0, 0) % QStringLiteral(" Center")));
              }
              else
              {
                // TRACON: Departure or approach from traconboundaries.json
                if(!prefixes.isEmpty())
                {
                  for(const QString& prefix : prefixes)
                  {
                    if(prefix != id)
                      ident.append(prefix);
                  }
                }
                ident.append(id);

                // TRACON
                QString suffix = propertiesObj.value(QStringLiteral("suffix")).toString();
                if(suffix == QStringLiteral("DEP"))
                {
                  // Departure
                  ident.append(QStringLiteral("DEP"));
                  insertAirspaceQuery->bindValue(QStringLiteral(":type"), QStringLiteral("D"));
                }
                else
                {
                  // Approach
                  ident.append(QStringLiteral("APP"));
                  insertAirspaceQuery->bindValue(QStringLiteral(":type"), QStringLiteral("A"));
                }

                ident.removeAll(QString());
                insertAirspaceQuery->bindValue(QStringLiteral(":name"), ident.join('_'));
                insertAirspaceQuery->bindValue(QStringLiteral(":description"), propertiesObj.value(QStringLiteral("name")).toString());
              }

              // Assign bounding rectangle
              Rect bounding = line.boundingRect();
              insertAirspaceQuery->bindValue(QStringLiteral(":max_lonx"), bounding.getEast());
              insertAirspaceQuery->bindValue(QStringLiteral(":max_laty"), bounding.getNorth());
              insertAirspaceQuery->bindValue(QStringLiteral(":min_lonx"), bounding.getWest());
              insertAirspaceQuery->bindValue(QStringLiteral(":min_laty"), bounding.getSouth());

              // Create geometry blob
              atools::fs::common::BinaryGeometry geo(atools::fs::util::correctBoundary(line));
              insertAirspaceQuery->bindValue(QStringLiteral(":geometry"), geo.writeToByteArray());

              insertAirspaceQuery->exec();
              insertAirspaceQuery->clearBoundValues();
              numAirspacesRead++;
            }
            else
              qWarning() << Q_FUNC_INFO << "No geometry found"
                         << "Ignoring all except first." << "id" << id << "region" << region << "division" << division;
          }
        }
      }
    } // if(error.error == QJsonParseError::NoError)
    else
      qWarning() << Q_FUNC_INFO << "Error reading" << filename << error.errorString() << "at offset" << error.offset;

    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2 (%3)").arg(filename).arg(file.errorString()).arg(file.error()));

  return numAirspacesRead > 0;
}

} // namespace userdata
} // namespace fs
} // namespace atools
