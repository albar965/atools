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

#include "fs/userdata/userdatamanager.h"

#include "sql/sqlutil.h"
#include "sql/sqlexport.h"
#include "sql/sqldatabase.h"
#include "sql/sqltransaction.h"
#include "util/csvreader.h"
#include "atools.h"
#include "geo/pos.h"
#include "fs/common/magdecreader.h"
#include "fs/util/fsutil.h"
#include "exception.h"

#include <QDir>
#include <QRegularExpression>
#include <QXmlStreamReader>

namespace atools {
namespace fs {
namespace userdata {

using atools::geo::Pos;
using atools::sql::SqlUtil;
using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::sql::SqlExport;
using atools::sql::SqlRecord;
using atools::sql::SqlTransaction;

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
  DESCRIPTION = 8,
  REGION = 9,
  VISIBLE_FROM = 10,
  LAST_EDIT = 11
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
  : DataManagerBase(sqlDb, "userdata", "userdata_id",
                    ":/atools/resources/sql/fs/userdata/create_user_schema.sql",
                    ":/atools/resources/sql/fs/userdata/drop_user_schema.sql",
                    "little_navmap_userdata_backup.csv")
{

}

UserdataManager::~UserdataManager()
{

}

void UserdataManager::updateSchema()
{
  if(!db->record(tableName).contains("temp"))
  {
    qDebug() << Q_FUNC_INFO;

    SqlTransaction transaction(db);
    // Add missing column and index
    db->exec("alter table " + tableName + " add column temp integer");
    db->exec("create index if not exists idx_userdata_temp on " + tableName + "(temp)");
    transaction.commit();
  }
}

void UserdataManager::clearTemporary()
{
  int tempRows = SqlUtil(db).rowCount(tableName, "temp = 1");

  qDebug() << Q_FUNC_INFO << "tempRows" << tempRows;

  if(tempRows > 0)
  {
    SqlTransaction transaction(db);
    SqlQuery deleteQuery("delete from " + tableName + " where temp = 1", db);
    deleteQuery.exec();
    transaction.commit();
  }
}

// VRP,  1NM NORTH SALERNO TOWN, 1NSAL, 40.6964,            14.785,         0,0, IT, FROM SOR VOR: 069° 22NM
// POI,  Cedar Butte lava flow,  POI,   43.4352891960911, -112.892122541337,0,0,   , photoreal areas
int UserdataManager::importCsv(const QString& filepath, atools::fs::userdata::Flags flags, QChar separator,
                               QChar escape)
{
  SqlTransaction transaction(db);

  // Autogenerate id
  QString insert = SqlUtil(db).buildInsertStatement(tableName, QString(), {idColumnName}, true);
  SqlQuery insertQuery(db);
  insertQuery.prepare(insert);

  QString absfilepath = QFileInfo(filepath).absoluteFilePath();
  QDateTime now = QDateTime::currentDateTime();

  int numImported = 0;
  QFile file(filepath);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    atools::util::CsvReader reader(separator, escape, true /* trim */);

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    int lineNum = 0;
    while(!stream.atEnd())
    {
      QString line = stream.readLine();

      if(lineNum == 0)
      {
        QString header = QString(line).replace(" ", QString()).toLower();
        if(flags & CSV_HEADER || header.startsWith("type,name,ident,latitude,longitude,elevation"))
        {
          lineNum++;
          // Ignore header
          continue;
        }
      }

      // Skip empty lines but add them if within an escaped field
      if(line.isEmpty() && !reader.isInEscape())
        continue;

      reader.readCsvLine(line);
      if(reader.isInEscape())
        // Still in an escaped line so continue to read unchanged until " shows the end of the field
        continue;

      const QStringList& values = reader.getValues();

      insertQuery.bindValue(":type", at(values, csv::TYPE));
      insertQuery.bindValue(":name", at(values, csv::NAME));
      insertQuery.bindValue(":ident", at(values, csv::IDENT));
      insertQuery.bindValue(":region", at(values, csv::REGION, true /* no warning */));
      insertQuery.bindValue(":description", at(values, csv::DESCRIPTION));
      insertQuery.bindValue(":tags", at(values, csv::TAGS));
      insertQuery.bindValue(":import_file_path", absfilepath);
      insertQuery.bindValue(":temp", 0);

      // YYYY-MM-DDTHH:mm:ss
      QDateTime lastEdit = QDateTime::fromString(at(values, csv::LAST_EDIT, true /* no warning */), Qt::ISODate);
      if(lastEdit.isValid())
        insertQuery.bindValue(":last_edit_timestamp", lastEdit.toString(Qt::ISODate));
      else
        insertQuery.bindValue(":last_edit_timestamp", now.toString(Qt::ISODate));

      bool ok;
      int visibleFrom = atools::roundToInt(at(values, csv::VISIBLE_FROM, true /* no warning */).toFloat(&ok));
      if(visibleFrom > 0 && ok)
        insertQuery.bindValue(":visible_from", visibleFrom);
      else
        insertQuery.bindValue(":visible_from", VISIBLE_FROM_DEFAULT_NM);

      insertQuery.bindValue(":altitude", at(values, csv::ALT));

      validateCoordinates(line, at(values, csv::LONX), at(values, csv::LATY));
      insertQuery.bindValue(":lonx", atFloat(values, csv::LONX, true));
      insertQuery.bindValue(":laty", atFloat(values, csv::LATY, true));
      insertQuery.exec();
      lineNum++;
      numImported++;
    }
    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
  transaction.commit();
  return numImported;
}

// I
// 1101 Version - data cycle 1704, build 20170325, metadata FixXP1101. NoCopyright (c) 2017 achwodu
//
// 50.88166700    12.58666700  PACEC PACEC ZZ
// 46.646819444 -123.722388889 AAYRR KSEA  K1 4530263
// 37.770908333 -122.082811111 AAAME ENRT  K2 4530263
int UserdataManager::importXplane(const QString& filepath)
{
  SqlTransaction transaction(db);
  QString insert = SqlUtil(db).buildInsertStatement(tableName,
                                                    QString(), {idColumnName, "description", "altitude"}, true);
  SqlQuery insertQuery(db);
  insertQuery.prepare(insert);

  QString absfilepath = QFileInfo(filepath).absoluteFilePath();
  QDateTime now = QDateTime::currentDateTime();

  int numImported = 0;
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

      insertQuery.bindValue(":type", "Waypoint");
      insertQuery.bindValue(":ident", at(cols, xp::IDENT));
      insertQuery.bindValue(":region", at(cols, xp::REGION));
      insertQuery.bindValue(":tags", at(cols, xp::AIRPORT));
      insertQuery.bindValue(":last_edit_timestamp", now.toString(Qt::ISODate));
      insertQuery.bindValue(":import_file_path", absfilepath);
      insertQuery.bindValue(":visible_from", VISIBLE_FROM_DEFAULT_NM);
      insertQuery.bindValue(":temp", 0);

      validateCoordinates(line, at(cols, xp::LONX), at(cols, xp::LATY));
      insertQuery.bindValue(":lonx", at(cols, xp::LONX));
      insertQuery.bindValue(":laty", at(cols, xp::LATY));
      insertQuery.exec();
      lineNum++;
      numImported++;
    }
    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
  transaction.commit();
  return numImported;
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
int UserdataManager::importGarmin(const QString& filepath)
{
  SqlTransaction transaction(db);
  QString insert = SqlUtil(db).buildInsertStatement(tableName,
                                                    QString(), {idColumnName, "description", "altitude", "region"},
                                                    true);
  SqlQuery insertQuery(db);
  insertQuery.prepare(insert);

  QString absfilepath = QFileInfo(filepath).absoluteFilePath();
  QDateTime now = QDateTime::currentDateTime();

  int numImported = 0;
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

      insertQuery.bindValue(":type", "Waypoint");
      insertQuery.bindValue(":name", at(cols, gm::NAME));
      insertQuery.bindValue(":ident", at(cols, gm::IDENT));
      insertQuery.bindValue(":last_edit_timestamp", now.toString(Qt::ISODate));
      insertQuery.bindValue(":import_file_path", absfilepath);
      insertQuery.bindValue(":visible_from", VISIBLE_FROM_DEFAULT_NM);
      insertQuery.bindValue(":temp", 0);

      validateCoordinates(line, at(cols, gm::LONX), at(cols, gm::LATY));
      insertQuery.bindValue(":lonx", at(cols, gm::LONX));
      insertQuery.bindValue(":laty", at(cols, gm::LATY));
      insertQuery.exec();
      lineNum++;
      numImported++;
    }
    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
  transaction.commit();
  return numImported;
}

int UserdataManager::exportCsv(const QString& filepath, const QVector<int>& ids, atools::fs::userdata::Flags flags,
                               QChar separator, QChar escape)
{
  // VRP,  1NM NORTH SALERNO TOWN, 1NSAL, 40.6964,            14.785,         0,0, IT, FROM SOR VOR: 069° 22NM
  // Check if the file ends with a \n or \r so we can add if necessary
  bool endsWithEol = atools::fileEndsWithEol(filepath);
  int numExported = 0;
  QFile file(filepath);
  if(file.open((flags & APPEND ? QIODevice::Append : QIODevice::WriteOnly) | QIODevice::Text))
  {
    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    SqlExport sqlExport;
    sqlExport.setSeparatorChar(separator);
    sqlExport.setEscapeChar(escape);
    sqlExport.setEndline(false);
    sqlExport.setHeader(flags & CSV_HEADER);
    sqlExport.setNumberPrecision(5);

    QueryWrapper query("select type as Type, name as Name, ident as Ident, laty as Latitude, lonx as Longitude, altitude as Elevation, "
                       "0 as \"Magnetic Declination\", tags as Tags, description as Description, region as Region, "
                       "cast(visible_from as integer) as \"Visible From\", "
                       "last_edit_timestamp as \"Last Edit\" from " + tableName,
                       db, ids,
                       idColumnName);

    if(!endsWithEol && (flags & APPEND))
      // Add needed linefeed for append
      stream << endl;

    bool first = true;

    query.exec();
    while(query.next())
    {
      if(first && flags & CSV_HEADER)
      {
        first = false;
        stream << sqlExport.getResultSetHeader(query.q.record()) << endl;
      }
      SqlRecord record = query.q.record();

      float magvar = 0.f;
      if(magDec->isValid())
        // Can be invalid if not database is loaded (no declination data) and backup is done
        magvar = magDec->getMagVar(Pos(record.valueFloat("Longitude"), record.valueFloat("Latitude")));

      // Need to cast otherwise it is not recognized as a floating point number
      record.setValue("Magnetic Declination", static_cast<double>(magvar));

      stream << sqlExport.getResultSetRow(record) << endl;
      numExported++;
    }

    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
  return numExported;
}

int UserdataManager::exportXplane(const QString& filepath, const QVector<int>& ids, atools::fs::userdata::Flags flags)
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

        // inFile.rename(inFile.fileName() + QString("_%1_lnm.bak").arg(QDateTime::currentSecsSinceEpoch()));
        inFile.remove();

        // Copy the temporary file back into place
        tempOutFile.rename(filepath);
      }
      else
        throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").
                                arg(inFile.fileName()).arg(inFile.errorString()));
    }
    else
      throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").
                              arg(tempOutFile.fileName()).arg(tempOutFile.errorString()));
  }

  int numExported = 0;
  QFile file(filepath);
  if(file.open((flags & APPEND ? QIODevice::Append : QIODevice::WriteOnly) | QIODevice::Text))
  {
    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    // I
    // 1100 Version - data cycle 1804, build 20180421, metadata FixXP1100. Created by Little Navmap Version 1.9.1.develop (revision 47ef66a) on 2018 04 21T13:25:52
    //

    if(!(flags & APPEND))
    {
      // Add file header
      stream << "I" << endl << "1100 Version - "
             << "data cycle " << QDateTime::currentDateTime().toString("yyMM") << ", "
             << "build " << QDateTime::currentDateTime().toString("yyyyMMdd") << ", "
             << "metadata FixXP1100. "
             << atools::programFileInfoNoDate() << "." << endl << endl;
    }

    QueryWrapper query(
      "select " + idColumnName + ", ident, name, tags, laty, lonx, altitude, tags, region from " + tableName,
      db, ids, idColumnName);
    // 50.88166700    12.58666700  PACEC PACEC ZZ
    // 46.646819444 -123.722388889 AAYRR KSEA  K1 4530263
    // 37.770908333 -122.082811111 AAAME ENRT  K2 4530263
    query.exec();
    while(query.next())
    {
      QString region = query.q.valueStr("region").toUpper();

      stream << QString::number(query.q.valueDouble("laty"), 'f', 8)
             << " " << QString::number(query.q.valueDouble("lonx"), 'f', 8)
             << " " << atools::fs::util::adjustIdent(query.q.valueStr("ident"), 5, query.q.valueInt(idColumnName))
             << " " << "ENRT" // Ignore airport here
             << " " << (region.isEmpty() ? "ZZ" : atools::fs::util::adjustRegion(region))
             << endl;
      numExported++;
    }

    stream << "99" << endl;

    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
  return numExported;
}

int UserdataManager::exportGarmin(const QString& filepath, const QVector<int>& ids, atools::fs::userdata::Flags flags)
{
  static const QRegularExpression ADJUST_NAME_REGEXP("[^A-Z0-9 /]");

  int numExported = 0;
  // Check for trailing EOL so we can add it if necessary
  bool endsWithEol = atools::fileEndsWithEol(filepath);

  QFile file(filepath);
  if(file.open((flags & APPEND ? QIODevice::Append : QIODevice::WriteOnly) | QIODevice::Text))
  {
    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    if(!endsWithEol && (flags & APPEND))
      stream << endl;

    QueryWrapper query("select " + idColumnName + ", ident, name,  laty, lonx from " + tableName, db, ids,
                       idColumnName);
    // MTHOOD,MT HOOD PEAK,45.3723,-121.69783
    // CRTRLK,CRATER LAKE,42.94683,-122.11083
    // EIFFEL,EIFFEL TOWER,48.858151,2.294384
    // OCEAN,,32.687356725,-51.45543634
    query.exec();
    while(query.next())
    {
      stream << atools::fs::util::adjustIdent(query.q.valueStr("ident"), 6, query.q.valueInt(idColumnName))
             << ","
             << query.q.valueStr("name").simplified().toUpper().replace(ADJUST_NAME_REGEXP, "").left(25)
             << ","
             << QString::number(query.q.valueDouble("laty"), 'f', 8)
             << ","
             << QString::number(query.q.valueDouble("lonx"), 'f', 8)
             << endl;
      numExported++;
    }

    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
  return numExported;
}

int UserdataManager::exportBgl(const QString& filepath, const QVector<int>& ids)
{
  // <?xml version="1.0"?>
  // <FSData version="9.0" xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xsi:noNamespaceSchemaLocation="bglcomp.xsd" >
  // <Waypoint lat="41.35184943" lon="-89.15309158" waypointType="IAF" magvar="0.0" waypointRegion="K6" waypointIdent="WLS">
  // </Waypoint>
  // </FSData>

  QFile xmlFile(filepath);
  int numExported = 0;

  if(xmlFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QXmlStreamWriter writer(&xmlFile);
    writer.setCodec("UTF-8");
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(4);
    writer.writeStartDocument("1.0");
    writer.writeStartElement("FSData");
    writer.writeAttribute("version", "9.0");
    writer.writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    writer.writeAttribute("xsi:noNamespaceSchemaLocation", "bglcomp.xsd");
    writer.writeComment(atools::programFileInfo());

    QueryWrapper query("select " + idColumnName + ", ident, region, tags, laty, lonx from " + tableName, db, ids,
                       idColumnName);
    query.exec();
    while(query.next())
    {
      QString region = query.q.valueStr("region").toUpper();
      if(region.size() != 2)
        region = "ZZ";

      writer.writeStartElement("Waypoint");
      writer.writeAttribute("lat", QString::number(query.q.valueDouble("laty"), 'f', 8));
      writer.writeAttribute("lon", QString::number(query.q.valueDouble("lonx"), 'f', 8));
      writer.writeAttribute("waypointType", "NAMED");
      writer.writeAttribute("waypointRegion", region);
      writer.writeAttribute("magvar",
                            QString::number(magDec->getMagVar(Pos(query.q.valueFloat("lonx"),
                                                                  query.q.valueFloat("laty"))), 'f', 8));
      writer.writeAttribute("waypointIdent",
                            atools::fs::util::adjustIdent(query.q.valueStr("ident"), 5,
                                                          query.q.valueInt(idColumnName)));
      writer.writeEndElement(); // Waypoint
      numExported++;
    }

    writer.writeEndElement(); // FSData
    writer.writeEndDocument();
  }
  return numExported;
}

} // namespace userdata
} // namespace fs
} // namespace atools
