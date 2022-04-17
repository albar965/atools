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

#include "fs/userdata/airspacereadervatsim.h"

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
      for(QJsonValue featureValue : doc.object().value("features").toArray())
      {
        QJsonObject featureObj = featureValue.toObject();
        QJsonObject propertiesObj = featureObj.value("properties").toObject();
        QJsonObject geometryObj = featureObj.value("geometry").toObject();

        // Used for name
        QString id = propertiesObj.value("id").toString();

        // Region and division only for logging
        QString region = propertiesObj.value("region").toString(); // only firboundaries.json
        QString division = propertiesObj.value("division").toString(); //// only firboundaries.json

        // Collect prefixes from array
        QStringList prefixes;
        for(QJsonValue prefix :  propertiesObj.value("prefix").toArray(QJsonArray())) // only traconboundaries.json
          prefixes.append(prefix.toString());

        // Skip unknown polygon types =====
        QString geometryType = geometryObj.value("type").toString();
        if(geometryType != "MultiPolygon")
        {
          qWarning() << Q_FUNC_INFO << "Unexpected polygon type" << geometryType
                     << "Only MultiPolygon allowed" << "id" << id << "region" << region << "division" << division;
          continue;
        }

        // Read array of polygons ==============
        for(QJsonValue geometryValue : geometryObj.value("coordinates").toArray())
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
            for(QJsonValue ringValue : ringArr.at(ringIndex).toArray())
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
              insertAirspaceQuery->bindValue(":boundary_id", airspaceId++);
              insertAirspaceQuery->bindValue(":file_id", fileId);

              // Unused fields here
              insertAirspaceQuery->bindNullStr(":restrictive_designation");
              insertAirspaceQuery->bindNullStr(":restrictive_type");
              insertAirspaceQuery->bindNullStr(":multiple_code");
              insertAirspaceQuery->bindValue(":time_code", "U");

              // Build boundary name
              QStringList ident;
              if(propertiesObj.contains("oceanic"))
              {
                // Center - only firboundaries.json
                ident.append(id.replace('-', '_').replace("__", "_") % "_CTR");
                ident.removeAll(QString());
                insertAirspaceQuery->bindValue(":type", "C");
                insertAirspaceQuery->bindValue(":name", ident.join('_'));
                insertAirspaceQuery->bindValue(":description", QString(id.section(QRegularExpression("[\\-_]"), 0, 0) % " Center"));
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
                QString suffix = propertiesObj.value("suffix").toString();
                if(suffix == "DEP")
                {
                  // Departure
                  ident.append("DEP");
                  insertAirspaceQuery->bindValue(":type", "D");
                }
                else
                {
                  // Approach
                  ident.append("APP");
                  insertAirspaceQuery->bindValue(":type", "A");
                }

                ident.removeAll(QString());
                insertAirspaceQuery->bindValue(":name", ident.join('_'));
                insertAirspaceQuery->bindValue(":description", propertiesObj.value("name").toString());
              }

              // Assign bounding rectangle
              Rect bounding = line.boundingRect();
              insertAirspaceQuery->bindValue(":max_lonx", bounding.getEast());
              insertAirspaceQuery->bindValue(":max_laty", bounding.getNorth());
              insertAirspaceQuery->bindValue(":min_lonx", bounding.getWest());
              insertAirspaceQuery->bindValue(":min_laty", bounding.getSouth());

              // Create geometry blob
              atools::fs::common::BinaryGeometry geo(line);
              insertAirspaceQuery->bindValue(":geometry", geo.writeToByteArray());

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
