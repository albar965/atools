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

#include "fs/userdata/userdatamanager.h"

#include "sql/sqldatabase.h"
#include "sql/sqlutil.h"
#include "sql/sqlscript.h"
#include "sql/sqlexport.h"
#include "sql/sqlquery.h"
#include "atools.h"
#include "geo/pos.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QTemporaryFile>
#include <QDir>
#include <QRegularExpression>
#include <QXmlStreamReader>

namespace atools {
namespace fs {
namespace userdata {

using atools::sql::SqlUtil;
using atools::sql::SqlDatabase;
using atools::sql::SqlScript;
using atools::sql::SqlQuery;
using atools::sql::SqlExport;
using atools::sql::SqlRecord;

/* Default visibility. Waypoint is shown on the map at a view distance below this value  */
const static int VISIBLE_FROM_DEFAULT_NM = 250;

namespace csv {
/* Column indexes in CSV format */
enum Index
{
  TYPE = 0,
  NAME = 1,
  IDENT = 2,
  LATY = 3,
  LONX = 4,
  ALT = 5,
  MAGVAR = 6,
  TAGS = 7,
  DESCRIPTION = 8
};

}

namespace xp {
/* Column indexes in X-Plane user_fix.dat format */
enum Index
{
  LATY = 0,
  LONX = 1,
  IDENT = 2,
  AIRPORT = 3,
  REGION = 4,
  FLAGS = 5
};

}

namespace gm {
/* Column indexes in Garmin GTN format */
enum Index
{
  IDENT = 0,
  NAME = 1,
  LATY = 2,
  LONX = 3
};

}

UserdataManager::UserdataManager(sql::SqlDatabase *sqlDb)
  : db(sqlDb)
{

}

UserdataManager::~UserdataManager()
{

}

bool UserdataManager::hasSchema()
{
  return SqlUtil(db).hasTable("userdata");
}

bool UserdataManager::hasData()
{
  return SqlUtil(db).hasTableAndRows("userdata");
}

void UserdataManager::createSchema()
{
  SqlScript script(db, true /*options->isVerbose()*/);

  script.executeScript(":/atools/resources/sql/fs/userdata/create_user_schema.sql");
  db->commit();
}

void UserdataManager::clearData()
{
  SqlQuery query("delete from userdata", db);
  query.exec();
  db->commit();
}

void UserdataManager::updateCoordinates(int id, const geo::Pos& position)
{
  SqlQuery query(db);
  query.prepare("update userdata set lonx = ?, laty = ? where userdata_id = ?");
  query.bindValue(0, position.getLonX());
  query.bindValue(1, position.getLatY());
  query.bindValue(2, id);
  query.exec();
}

void UserdataManager::updateField(const QString& column, const QVector<int>& ids, const QVariant& value)
{
  SqlQuery query(db);
  query.prepare("update userdata set " + column + " = ? where userdata_id = ?");

  for(int id : ids)
  {
    // Update field for all rows with the given id
    query.bindValue(0, value);
    query.bindValue(1, id);
    query.exec();
  }
}

void UserdataManager::insertByRecord(const sql::SqlRecord& record)
{
  QVariantList vals = record.values();
  SqlQuery query(db);
  query.prepare("insert into userdata (" + record.fieldNames().join(", ") + ") " +
                "values(" + QString("?, ").repeated(vals.size() - 1) + " ?)");
  for(int i = 0; i < vals.size(); i++)
    query.bindValue(i, vals.at(i));
  query.exec();
}

void UserdataManager::updateByRecord(sql::SqlRecord record, const QVector<int>& ids)
{
  if(record.contains("userdata_id"))
    // Get rid of id column - it is not needed here
    record.remove(record.indexOf("userdata_id"));

  SqlQuery query(db);
  query.prepare("update userdata set " + record.fieldNames().join(" = ?, ") + " = ? where userdata_id = ?");

  QVariantList vals = record.values();
  for(int id : ids)
  {
    // For each row with given id ...
    for(int i = 0; i < vals.size(); i++)
      // ... update all given columns with given values
      query.bindValue(i, vals.at(i));
    query.bindValue(vals.size(), id);
    query.exec();
  }
}

void UserdataManager::removeRows(const QVector<int> ids)
{
  SqlQuery query(db);
  query.prepare("delete from userdata where userdata_id = ?");

  for(int id : ids)
  {
    query.bindValue(0, id);
    query.exec();
  }
}

void UserdataManager::records(QVector<SqlRecord>& records, const QVector<int> ids)
{
  SqlQuery query(db);
  query.prepare("select * from userdata where userdata_id = ?");

  for(int id : ids)
  {
    query.bindValue(0, id);
    query.exec();
    if(query.next())
      records.append(query.record());
  }
}

SqlRecord UserdataManager::record(int id)
{
  QVector<SqlRecord> recs;
  records(recs, {id});
  return recs.isEmpty() ? SqlRecord() : recs.first();
}

void UserdataManager::emptyRecord(SqlRecord& record)
{
  record = db->record("userdata");
}

SqlRecord UserdataManager::emptyRecord()
{
  SqlRecord rec;
  emptyRecord(rec);
  return rec;
}

void UserdataManager::commit()
{
  db->commit();
}

// create table userdata
// (
// userdata_id integer primary key,
// type varchar(10) collate nocase,    -- VRP, POI, OBS, IFR, etc.
// name varchar(200) collate nocase,
// ident varchar(10) collate nocase,
// region varchar(4) collate nocase,
// description varchar(1024) collate nocase,
// region varchar(4) collate nocase,
// last_edit_timestamp varchar(100) not null,   -- Timestamp of last edit (i.e. "2016-07-05T15:45:30.396")
// import_timestamp varchar(100) not null,   -- Timestamp of last loading (i.e. "2016-07-05T15:45:30.396")
// import_file_path varchar(512) not null,   -- Timestamp of last loading (i.e. "2016-07-05T15:45:30.396")
// altitude integer,
// lonx double not null,
// laty double not null
// );

// VRP,  1NM NORTH SALERNO TOWN, 1NSAL, 40.6964,            14.785,         0,0, IT, FROM SOR VOR: 069° 22NM
// POI,  Cedar Butte lava flow,  POI,   43.4352891960911, -112.892122541337,0,0,   , photoreal areas
void UserdataManager::importCsv(const QString& filepath, atools::fs::userdata::Flags flags, QChar separator,
                                QChar escape)
{
  // Autogenerate id
  QString insert = SqlUtil(db).buildInsertStatement("userdata", QString(), {"userdata_id"}, true);
  SqlQuery insertQuery(db);
  insertQuery.prepare(insert);

  QString absfilepath = QFileInfo(filepath).absoluteFilePath();
  QDateTime now = QDateTime::currentDateTime();
  QStringList values;

  QFile file(filepath);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    int lineNum = 0;
    while(!stream.atEnd())
    {
      if(flags & CSV_HEADER && lineNum == 0)
        // Ignore header
        continue;

      QString line = stream.readLine().simplified();
      if(line.isEmpty())
        continue;

      readCsvLine(values, line, separator, escape);

      insertQuery.bindValue(":type", at(values, csv::TYPE));
      insertQuery.bindValue(":name", at(values, csv::NAME));
      insertQuery.bindValue(":ident", at(values, csv::IDENT));
      insertQuery.bindValue(":description", at(values, csv::DESCRIPTION));
      insertQuery.bindValue(":mag_var", at(values, csv::MAGVAR));
      insertQuery.bindValue(":tags", at(values, csv::TAGS));
      insertQuery.bindValue(":last_edit_timestamp", now);
      insertQuery.bindValue(":import_timestamp", now);
      insertQuery.bindValue(":import_file_path", absfilepath);
      insertQuery.bindValue(":visible_from", VISIBLE_FROM_DEFAULT_NM);
      insertQuery.bindValue(":altitude", at(values, csv::ALT));
      insertQuery.bindValue(":lonx", at(values, csv::LONX));
      insertQuery.bindValue(":laty", at(values, csv::LATY));
      insertQuery.exec();
      lineNum++;
    }
    db->commit();
    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
}

// I
// 1101 Version - data cycle 1704, build 20170325, metadata FixXP1101. NoCopyright (c) 2017 achwodu
//
// 50.88166700    12.58666700  PACEC PACEC ZZ
// 46.646819444 -123.722388889 AAYRR KSEA  K1 4530263
// 37.770908333 -122.082811111 AAAME ENRT  K2 4530263
void UserdataManager::importXplane(const QString& filepath)
{
  QString insert = SqlUtil(db).buildInsertStatement("userdata",
                                                    QString(), {"userdata_id", "description", "altitude"}, true);
  SqlQuery insertQuery(db);
  insertQuery.prepare(insert);

  QString absfilepath = QFileInfo(filepath).absoluteFilePath();
  QDateTime now = QDateTime::currentDateTime();

  QFile file(filepath);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    QString line = stream.readLine().simplified();
    if(line != "I" && line != "A")
      throw atools::Exception("File is not an X-Plane user_fix.dat file.");

    line = stream.readLine().simplified();
    if(!line.startsWith("11"))
      throw atools::Exception("File is not an X-Plane user_fix.dat file.");

    line = stream.readLine().simplified();
    if(!line.isEmpty())
      throw atools::Exception("File is not an X-Plane user_fix.dat file.");

    int lineNum = 0;
    while(!stream.atEnd())
    {
      line = stream.readLine().simplified();
      if(line.isEmpty())
        continue;
      if(line == "99")
        break;

      QStringList cols = line.split(" ");

      insertQuery.bindValue(":type", "WPT");
      insertQuery.bindValue(":name", at(cols, xp::IDENT));
      insertQuery.bindValue(":ident", at(cols, xp::IDENT));
      // Put airport/ENRT and region into the tags field
      insertQuery.bindValue(":tags", at(cols, xp::AIRPORT) + "/" + at(cols, xp::REGION));
      insertQuery.bindValue(":last_edit_timestamp", now);
      insertQuery.bindValue(":import_timestamp", now);
      insertQuery.bindValue(":import_file_path", absfilepath);
      insertQuery.bindValue(":visible_from", VISIBLE_FROM_DEFAULT_NM);
      insertQuery.bindValue(":lonx", at(cols, xp::LONX));
      insertQuery.bindValue(":laty", at(cols, xp::LATY));
      insertQuery.exec();
      lineNum++;
    }
    db->commit();
    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
}

// 3.2 File Contents
// The user waypoint file is a comma-separated values (CSV) file. Each row in the file represents a unique user waypoint.
// There must be four columns of data in the file:
// 1. Waypoint name 2. Waypoint comment 3. Latitude 4. Longitude
// A comma (,) must be used between each value and a carriage return must be inserted after the longitude.
// The waypoint name can be up to 10 numbers or capital letters; however, the GTN will shorten the name to the first 6 characters.
// No special characters or symbols can be used.
// The waypoint comment can be up to 25 numbers, capital letters, spaces, or forward slash (/) characters.
// The comment is not used to define the position of the waypoint but is displayed when selecting waypoints to
// provide additional context to the pilot. The latitude is specified in decimal degrees.
// For southern latitudes, include a dash (-) in front of the latitude value. Up to two digits can be provided to the
// left of the decimal point and nine to the right of the decimal point. (Example: -12.123456789)
// The longitude is specified in decimal degrees. For western longitudes, include a dash (-) in front of the longitude value.
// Up to three digits can be provided to the left of the decimal point and eight to the right of the decimal point.
// (Example: -123.12345678) Note: If a waypoint to be imported is located within 0.001° (latitude and longitude) of an
// existing user waypoint in the GTN, the existing waypoint and name will be reused.
// 3.3 User Waypoint File Examples file:
// MTHOOD,MT HOOD PEAK,45.3723,-121.69783
// CRTRLK,CRATER LAKE,42.94683,-122.11083
// EIFFEL,EIFFEL TOWER,48.858151,2.294384
// OCEAN,,32.687356725,-51.45543634
void UserdataManager::importGarmin(const QString& filepath)
{
  QString insert = SqlUtil(db).buildInsertStatement("userdata",
                                                    QString(), {"userdata_id", "description", "altitude"}, true);
  SqlQuery insertQuery(db);
  insertQuery.prepare(insert);

  QString absfilepath = QFileInfo(filepath).absoluteFilePath();
  QDateTime now = QDateTime::currentDateTime();

  QFile file(filepath);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    int lineNum = 0;
    while(!stream.atEnd())
    {
      QString line = stream.readLine().simplified();
      if(line.isEmpty())
        continue;

      QStringList cols = line.split(",");

      insertQuery.bindValue(":type", "WPT");
      insertQuery.bindValue(":name", at(cols, gm::NAME));
      insertQuery.bindValue(":ident", at(cols, gm::IDENT));
      insertQuery.bindValue(":last_edit_timestamp", now);
      insertQuery.bindValue(":import_timestamp", now);
      insertQuery.bindValue(":import_file_path", absfilepath);
      insertQuery.bindValue(":visible_from", VISIBLE_FROM_DEFAULT_NM);
      insertQuery.bindValue(":lonx", at(cols, gm::LONX));
      insertQuery.bindValue(":laty", at(cols, gm::LATY));
      insertQuery.exec();
      lineNum++;
    }
    db->commit();
    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
}

void UserdataManager::exportCsv(const QString& filepath, atools::fs::userdata::Flags flags, QChar separator,
                                QChar escape)
{
  // VRP,  1NM NORTH SALERNO TOWN, 1NSAL, 40.6964,            14.785,         0,0, IT, FROM SOR VOR: 069° 22NM
  // Check if the file ends with a \n or \r so we can add if necessary
  bool endsWithEol = atools::fileEndsWithEol(filepath);

  QFile file(filepath);
  if(file.open((flags & APPEND ? QIODevice::Append : QIODevice::WriteOnly) | QIODevice::Text))
  {
    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    SqlExport sqlExport;
    sqlExport.setSeparatorChar(separator);
    sqlExport.setEscapeChar(escape);
    sqlExport.setHeader(flags & CSV_HEADER);

    SqlQuery query("select type, name, ident, laty, lonx, altitude as elevation, mag_var, tags, description "
                   "from userdata", db);

    if(!endsWithEol && flags & APPEND)
      // Add needed linefeed for append
      stream << endl;

    sqlExport.printResultSet(query, stream);

    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
}

void UserdataManager::exportXplane(const QString& filepath, atools::fs::userdata::Flags flags)
{
  if(flags & APPEND)
  {
    // Copy the whole file into a new one and remove the trailing 99
    QFile tempOutFile(QFileInfo(filepath).path() + QDir::separator() +
                      QString("lnm_user_fix_dat_%1").arg(QDateTime::currentSecsSinceEpoch()));
    if(tempOutFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
      QFile inFile(filepath);

      if(inFile.open(QIODevice::ReadOnly | QIODevice::Text))
      {
        QTextStream tempOutStream(&tempOutFile);
        tempOutStream.setCodec("UTF-8");

        QTextStream inStream(&inFile);
        inStream.setCodec("UTF-8");

        // Copy contents exept the trailing 99 new file always has a trailing EOL
        while(!inStream.atEnd())
        {
          QString line = inStream.readLine();
          if(line.simplified() == "99")
            break;
          tempOutStream << line << endl;
        }

        inFile.close();
        tempOutFile.close();

        // Create a backup of the original file
        inFile.rename(inFile.fileName() + QString("_%1_lnm.bak").arg(QDateTime::currentSecsSinceEpoch()));

        // Copy the temporary file back into place
        tempOutFile.copy(filepath);
      }
      else
        throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").
                                arg(inFile.fileName()).arg(inFile.errorString()));
    }
    else
      throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").
                              arg(tempOutFile.fileName()).arg(tempOutFile.errorString()));
  }

  QFile file(filepath);
  if(file.open((flags & APPEND ? QIODevice::Append : QIODevice::WriteOnly) | QIODevice::Text))
  {
    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    // I
    // 1101 Version - data cycle 1704, build 20170325, metadata FixXP1101. NoCopyright (c) 2017 achwodu
    //

    if(!(flags & APPEND))
      // Add file header
      stream << "I" << endl << "1101 Version - data cycle any. " << atools::programFileInfo() << endl << endl;

    SqlQuery query("select ident, name, tags, laty, lonx, altitude, tags from userdata", db);
    // 50.88166700    12.58666700  PACEC PACEC ZZ
    // 46.646819444 -123.722388889 AAYRR KSEA  K1 4530263
    // 37.770908333 -122.082811111 AAAME ENRT  K2 4530263
    query.exec();
    while(query.next())
    {
      QString tags = query.valueStr("tags").toUpper();
      QString airport = tags.section('/', 0, 0);
      QString region = tags.section('/', 1, 1);

      stream << query.valueDouble("laty")
             << " " << query.valueDouble("lonx")
             << " " << adjustIdent(query.valueStr("ident"))
             << " " << (airport.isEmpty() ? "ENRT" : airport)
             << " " << (region.isEmpty() ? "ZZ" : region)
             << endl;
    }

    if(flags & APPEND)
      stream << "99" << endl;

    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
}

void UserdataManager::exportGarmin(const QString& filepath, atools::fs::userdata::Flags flags)
{
  // Check for trailing EOL so we can add it if necessary
  bool endsWithEol = atools::fileEndsWithEol(filepath);

  QFile file(filepath);
  if(file.open((flags & APPEND ? QIODevice::Append : QIODevice::WriteOnly) | QIODevice::Text))
  {
    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    if(!endsWithEol && flags & APPEND)
      stream << endl;

    SqlQuery query("select ident, name,  laty, lonx from userdata", db);
    // MTHOOD,MT HOOD PEAK,45.3723,-121.69783
    // CRTRLK,CRATER LAKE,42.94683,-122.11083
    // EIFFEL,EIFFEL TOWER,48.858151,2.294384
    // OCEAN,,32.687356725,-51.45543634
    query.exec();
    while(query.next())
      stream << adjustIdent(query.valueStr("ident"))
             << ","
             << query.valueStr("name").simplified().toUpper().replace(QRegularExpression("[^A-Z0-9 /]"), "").left(25)
             << ","
             << query.valueFloat("laty")
             << ","
             << query.valueFloat("lonx")
             << "\r";

    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
}

void UserdataManager::exportBgl(const QString& filepath)
{
  // <?xml version="1.0"?>
  // <FSData version="9.0" xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xsi:noNamespaceSchemaLocation="bglcomp.xsd" >
  // <Waypoint lat="41.35184943" lon="-89.15309158" waypointType="IAF" magvar="0.0" waypointRegion="K6" waypointIdent="WLS">
  // </Waypoint>
  // </FSData>

  QFile xmlFile(filepath);

  if(xmlFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QString xmlString;

    QXmlStreamWriter writer(&xmlString);
    writer.setCodec("UTF-8");
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(4);
    writer.writeStartDocument("1.0");
    writer.writeStartElement("FSData");
    writer.writeAttribute("version", "9.0");
    writer.writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    writer.writeAttribute("xsi:noNamespaceSchemaLocation", "bglcomp.xsd");
    writer.writeComment(atools::programFileInfo());

    SqlQuery query("select ident, tags, laty, lonx, from userdata", db);
    query.exec();
    while(query.next())
    {
      QString region = query.valueStr("tags").toUpper();
      if(region.size() != 2)
        region = "ZZ";

      writer.writeStartElement("Waypoint");
      writer.writeAttribute("lat", query.valueStr("laty"));
      writer.writeAttribute("lon", query.valueStr("lonx"));
      writer.writeAttribute("waypointType", "NAMED");
      writer.writeAttribute("mag_var", query.valueStr("NAMED"));
      writer.writeAttribute("waypointRegion", region);
      writer.writeAttribute("waypointIdent", adjustIdent(query.valueStr("ident")));
      writer.writeEndElement(); // Waypoint
    }

    writer.writeEndElement(); // FSData
    writer.writeEndDocument();
  }
}

QString UserdataManager::at(const QStringList& line, int index)
{
  if(index < line.size())
    return line.at(index);

  qWarning() << "Index" << index << "not found in file";
  return QString();
}

QString UserdataManager::adjustIdent(QString ident)
{
  return ident.toUpper().replace(QRegularExpression("[^A-Z0-9]"), "").left(5);
}

} // namespace userdata
} // namespace fs
} // namespace atools
