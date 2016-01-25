/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include "fs/ap/airportloader.h"

#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlscript.h"
#include "sql/sqlutil.h"
#include "settings/settings.h"

#include <QFile>

namespace atools {
namespace fs {
namespace ap {

using atools::sql::SqlDatabase;
using atools::sql::SqlScript;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;

AirportLoader::AirportLoader(SqlDatabase *sqlDb)
  : db(sqlDb)
{
  query = new SqlQuery(db);
}

AirportLoader::~AirportLoader()
{
  delete query;
}

void AirportLoader::loadAirports(const QString& filename)
{
  QFile xmlFile(filename);

  if(xmlFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    using atools::settings::Settings;
    SqlScript script(db);

    if(!SqlUtil(db).hasTable("airport"))
      // Drop if exists and create tables
      script.executeScript(Settings::getOverloadedPath(":/atools/resources/sql/ap/create_schema.sql"));

    reader.setDevice(&xmlFile);

    reader.readNextStartElement();

    if(reader.name() == "data")
    {
      numLoaded = 0;
      // Use insert or replace
      query->prepare(SqlUtil(db).buildInsertStatement("airport", "or replace"));
      readData();
    }
    else
      reader.raiseError(QObject::tr("The file is not an runways.xml file. Element \"data\" not found."));

    if(reader.error() == QXmlStreamReader::NoError)
    {
      db->commit();
      script.executeScript(Settings::getOverloadedPath(":/atools/resources/sql/ap/finish_schema.sql"));
      db->commit();
    }
    else
    {
      db->rollback();
      throw Exception(QString(QObject::tr("Error reading runways.xml file \"%1\". Reason: %2.")).
                      arg(xmlFile.fileName()).arg(reader.errorString()));
    }
  }
  else
  {
    db->rollback();
    throw Exception(QString(QObject::tr("Cannot open runways.xml file \"%1\". Reason: %2.")).
                    arg(xmlFile.fileName()).arg(xmlFile.errorString()));
  }
}

void AirportLoader::readData()
{
  db->exec("delete from airport");

  while(reader.readNextStartElement())
  {
    if(reader.name() == "ICAO")
      readIcao();
    else
      // Error will be recognized later
      reader.raiseError(QObject::tr("The file is not an runways.xml file. Element \"ICAO\" not found."));
  }
}

void AirportLoader::readIcao()
{
  QStringRef id = reader.attributes().value("id");
  query->bindValue(":icao", id.toString());

  bool hasLights = false, hasIls = false;
  int maxRunwayLength = 0;

  while(reader.readNextStartElement())
  {
    // Read only a part of the elementss
    QStringRef name = reader.name();
    if(name == "ICAOName")
      query->bindValue(":name", reader.readElementText());
    else if(name == "City")
      query->bindValue(":city", reader.readElementText());
    else if(name == "State")
      query->bindValue(":state", reader.readElementText());
    else if(name == "Country")
      query->bindValue(":country", reader.readElementText());
    else if(name == "Longitude")
      query->bindValue(":longitude", reader.readElementText().toDouble());
    else if(name == "Latitude")
      query->bindValue(":latitude", reader.readElementText().toDouble());
    else if(name == "Altitude")
      query->bindValue(":altitude", reader.readElementText().toInt());
    else if(name == "Runway")
      while(reader.readNextStartElement())
      {
        QStringRef rName = reader.name();
        if(rName == "Len")
          maxRunwayLength = qMax(reader.readElementText().toInt(), maxRunwayLength);
        else if(rName == "ILSFreq")
        {
          if(!reader.readElementText().trimmed().isEmpty())
            hasIls = true;
        }
        else if(rName == "EdgeLights")
        {
          if(reader.readElementText().trimmed() != "NONE")
            hasLights = true;
        }
        else
          reader.skipCurrentElement();
      }
    else
      reader.skipCurrentElement();
    numLoaded++;
  }
  query->bindValue(":max_runway_length", maxRunwayLength);
  query->bindValue(":has_lights", hasLights ? 1 : 0);
  query->bindValue(":has_ils", hasIls ? 1 : 0);
  query->exec();
}

void AirportLoader::dropDatabase()
{
  using atools::settings::Settings;

  SqlScript script(db);
  script.executeScript(Settings::getOverloadedPath(":/atools/resources/sql/ap/drop_schema.sql"));
  db->commit();
}

} // namespace ap
} // namespace fs
} // namespace atools
