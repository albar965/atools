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

#include "atools.h"
#include "exception.h"
#include "fs/common/magdecreader.h"
#include "fs/util/fsutil.h"
#include "geo/pos.h"
#include "sql/sqldatabase.h"
#include "sql/sqlexport.h"
#include "sql/sqltransaction.h"
#include "sql/sqlutil.h"
#include "util/csvreader.h"

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

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::endl;
#endif

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
  LAST_EDIT = 11,
  IMPORT_FILENAME = 12
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
  FLAGS = 5,
  NAME = 6
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
                    ":/atools/resources/sql/fs/userdata/create_user_schema_undo.sql",
                    ":/atools/resources/sql/fs/userdata/drop_user_schema.sql")
{

}

UserdataManager::~UserdataManager()
{

}

void UserdataManager::updateSchema()
{
  if(!db->record(tableName).contains("temp"))
  {
    qDebug() << Q_FUNC_INFO << "Adding temp colum to userdata";

    SqlTransaction transaction(db);
    // Add missing column and index
    addColumnIf("temp ", "integer");
    db->exec("create index if not exists idx_userdata_temp on " + tableName + "(temp)");
    transaction.commit();
  }

  DataManagerBase::updateUndoSchema();
}

void UserdataManager::clearTemporary()
{
  qDebug() << Q_FUNC_INFO;
  SqlTransaction transaction(db);
  deleteRows("temp", 1);
  transaction.commit();
}

// VRP,  1NM NORTH SALERNO TOWN, 1NSAL, 40.6964,            14.785,         0,0, IT, FROM SOR VOR: 069° 22NM
// POI,  Cedar Butte lava flow,  POI,   43.4352891960911, -112.892122541337,0,0,   , photoreal areas
int UserdataManager::importCsv(const QString& filepath, atools::fs::userdata::Flags flags, QChar separator,
                               QChar escape)
{
  int numImported = 0;
  QFile file(filepath);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    SqlTransaction transaction(db);

    int id = getCurrentId() + 1;
    preUndoBulkInsert(id);
    QString idBinding(":" + idColumnName);

    // Autogenerate id
    QString insert = SqlUtil(db).buildInsertStatement(tableName, QString(), QStringList(), true /* namedBindings */);
    SqlQuery insertQuery(db);
    insertQuery.prepare(insert);

    QString absfilepath = QFileInfo(filepath).absoluteFilePath();
    QDateTime now = QDateTime::currentDateTime();

    atools::util::CsvReader reader(separator, escape, true /* trim */);

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    int lineNum = 0;
    while(!stream.atEnd())
    {
      QString line = stream.readLine();

      if(lineNum == 0)
      {
        QString header = QString(line).simplified().replace(' ', QString()).replace('"', QString()).toLower();
        if(flags & CSV_HEADER || header.startsWith("type,name,ident,latitude,longitude"))
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

      insertQuery.bindValue(idBinding, id++);
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

      validateCoordinates(line, at(values, csv::LONX), at(values, csv::LATY), false /* checkNull */);
      insertQuery.bindValue(":lonx", atFloat(values, csv::LONX, true));
      insertQuery.bindValue(":laty", atFloat(values, csv::LATY, true));
      insertQuery.exec();
      lineNum++;
      numImported++;
    }
    file.close();

    postUndoBulkInsert();
    transaction.commit();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));

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
  int numImported = 0;
  QFile file(filepath);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    SqlTransaction transaction(db);

    int id = getCurrentId() + 1;
    preUndoBulkInsert(id);
    QString idBinding(":" + idColumnName);

    QString insert = SqlUtil(db).buildInsertStatement(tableName, QString(), {"description", "altitude"}, true /* namedBindings */);
    SqlQuery insertQuery(db);
    insertQuery.prepare(insert);

    QString absfilepath = QFileInfo(filepath).absoluteFilePath();
    QDateTime now = QDateTime::currentDateTime();

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    QString line = stream.readLine().simplified();
    if(line != "I" && line != "A")
      throw atools::Exception(tr("File is not an X-Plane user_fix.dat file."));

    line = stream.readLine().simplified();
    if(!line.startsWith("11") && !line.startsWith("12"))
      throw atools::Exception(tr("File is not an X-Plane user_fix.dat file."));

    line = stream.readLine().simplified();
    if(!line.isEmpty())
      throw atools::Exception(tr("File is not an X-Plane user_fix.dat file."));

    while(!stream.atEnd())
    {
      line = stream.readLine().simplified();
      if(line.isEmpty())
        continue;
      if(line == "99")
        break;

      QStringList cols = line.split(' ');

      // XP12 "51.801667   -8.573889  VP001 ENRT EI 2105430 HALFWAY ROUTE"
      // XP11 "46.646819444 -123.722388889 AAYRR KSEA  K1 4530263"
      QStringList tags;
      tags.append(at(cols, xp::AIRPORT, true /* nowarn */));
      tags.append(atools::fs::util::waypointFlagsFromXplane(at(cols, xp::FLAGS, true /* nowarn */)).replace(' ', '_'));
      tags.removeAll(QString());

      insertQuery.bindValue(idBinding, id++);
      insertQuery.bindValue(":type", "Waypoint");
      insertQuery.bindValue(":ident", at(cols, xp::IDENT));
      insertQuery.bindValue(":region", at(cols, xp::REGION));
      insertQuery.bindValue(":tags", tags.join(' '));
      insertQuery.bindValue(":name", cols.mid(xp::NAME).join(' ')); // Get rest of line as name
      insertQuery.bindValue(":last_edit_timestamp", now.toString(Qt::ISODate));
      insertQuery.bindValue(":import_file_path", absfilepath);
      insertQuery.bindValue(":visible_from", VISIBLE_FROM_DEFAULT_NM);
      insertQuery.bindValue(":temp", 0);

      validateCoordinates(line, at(cols, xp::LONX), at(cols, xp::LATY), false /* checkNull */);
      insertQuery.bindValue(":lonx", at(cols, xp::LONX));
      insertQuery.bindValue(":laty", at(cols, xp::LATY));
      insertQuery.exec();
      numImported++;
    }
    file.close();

    postUndoBulkInsert();
    transaction.commit();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
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
  int numImported = 0;
  QFile file(filepath);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    SqlTransaction transaction(db);
    int id = getCurrentId() + 1;
    preUndoBulkInsert(id);
    QString idBinding(":" + idColumnName);

    QString insert = SqlUtil(db).buildInsertStatement(tableName, QString(),
                                                      {"description", "altitude", "region"}, true /* namedBindings */);
    SqlQuery insertQuery(db);
    insertQuery.prepare(insert);

    QString absfilepath = QFileInfo(filepath).absoluteFilePath();
    QDateTime now = QDateTime::currentDateTime();

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    while(!stream.atEnd())
    {
      QString line = stream.readLine().simplified();
      if(line.isEmpty())
        continue;

      QStringList cols = line.split(",");

      insertQuery.bindValue(idBinding, id++);
      insertQuery.bindValue(":type", "Waypoint");
      insertQuery.bindValue(":name", at(cols, gm::NAME));
      insertQuery.bindValue(":ident", at(cols, gm::IDENT));
      insertQuery.bindValue(":last_edit_timestamp", now.toString(Qt::ISODate));
      insertQuery.bindValue(":import_file_path", absfilepath);
      insertQuery.bindValue(":visible_from", VISIBLE_FROM_DEFAULT_NM);
      insertQuery.bindValue(":temp", 0);

      validateCoordinates(line, at(cols, gm::LONX), at(cols, gm::LATY), false /* checkNull */);
      insertQuery.bindValue(":lonx", at(cols, gm::LONX));
      insertQuery.bindValue(":laty", at(cols, gm::LATY));
      insertQuery.exec();
      numImported++;
    }
    file.close();

    postUndoBulkInsert();
    transaction.commit();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
  return numImported;
}

int UserdataManager::exportCsv(const QString& filepath, const QVector<int>& ids, atools::fs::userdata::Flags flags, QChar separator,
                               QChar escape) const
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

    // Use query wrapper to automatically use passed ids or all rows
    QueryWrapper query("select type as Type, "
                       "name as Name, "
                       "ident as Ident, "
                       "laty as Latitude, lonx as Longitude, altitude as Elevation, "
                       "0 as \"Magnetic Declination\", "
                       "tags as Tags, "
                       "description as Description, "
                       "region as Region, "
                       "cast(visible_from as integer) as \"Visible From\", "
                       "last_edit_timestamp as \"Last Edit\", "
                       "import_file_path as \"Import Filename\" from " + tableName,
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
        // Write header
        first = false;
        stream << sqlExport.getResultSetHeader(query.query.record()) << endl;
      }
      SqlRecord record = query.query.record();

      float magvar = 0.f;
      if(magDec->isValid())
        // Can be invalid if not database is loaded (no declination data) and backup is done
        magvar = magDec->getMagVar(Pos(record.valueFloat("Longitude"), record.valueFloat("Latitude")));

      // Need to cast otherwise it is not recognized as a floating point number
      record.setValue("Magnetic Declination", static_cast<double>(magvar));

      // Write row
      stream << sqlExport.getResultSetRow(record) << endl;
      numExported++;
    }

    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
  return numExported;
}

int UserdataManager::exportXplane(const QString& filepath, const QVector<int>& ids, atools::fs::userdata::Flags flags, bool xp12)
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
      stream << "I" << endl << (xp12 ? "1200" : "1100") << " Version - "
             << "data cycle " << QLocale(QLocale::C).toString(QDateTime::currentDateTime(), "yyMM") << ", "
             << "build " << QLocale(QLocale::C).toString(QDateTime::currentDateTime(), "yyyyMMdd") << ", "
             << "metadata FixXP" << (xp12 ? "1200" : "1100") << ". "
             << atools::programFileInfoNoDate() << "." << endl << endl;
    }

    QueryWrapper query("select " + idColumnName + ", ident, name, tags, laty, lonx, altitude, tags, region from " + tableName, db, ids,
                       idColumnName);

    // X-Plane 11
    // I
    // 1101 Version - data cycle 1704, build 20170325, metadata FixXP1101.
    //
    // 48.90000000 15.30833300 PABFO PABFO ZZ
    // 50.88166700    12.58666700  PACEC PACEC ZZ
    // 46.646819444 -123.722388889 AAYRR KSEA  K1 4530263
    // 37.770908333 -122.082811111 AAAME ENRT  K2 4530263

    // X-Plane 12
    // I
    // 1200 Version - data cycle 1234, build 1234, metadata FixXP1200. blah blah blah
    //
    // 51.801667   -8.573889  VP001 ENRT EI 2105430 HALFWAY ROUTE
    // 51.816389   -8.390833  VP002 ENRT EI 2105430 CARRIGALINE
    // 99

    query.exec();
    while(query.next())
    {
      QString region = query.query.valueStr("region").toUpper();

      stream << QString::number(query.query.valueDouble("laty"), 'f', 8)
             << " " << QString::number(query.query.valueDouble("lonx"), 'f', 8)
             << " " << atools::fs::util::adjustIdent(query.query.valueStr("ident"), 5, query.query.valueInt(idColumnName));

      if(xp12)
      {
        QString airportIdent = query.query.valueStr("tags").section(' ', 0, 0);
        if(airportIdent.isEmpty())
          airportIdent = "ENRT"; // En-route
        stream << " " << airportIdent;
      }
      else
        stream << " " << "ENRT"; // Ignore airport here

      stream << " " << (region.isEmpty() ? "ZZ" : atools::fs::util::adjustRegion(region));

      if(xp12)
      {
        // Use VFR waypoint as default if nothing given
        stream << " " << atools::fs::util::waypointFlagsToXplane(query.query.valueStr("tags").section(' ', 1, 1), "2105430");
        stream << " " << query.query.valueStr("name");
      }

      stream << endl;
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
      stream << atools::fs::util::adjustIdent(query.query.valueStr("ident"), 6, query.query.valueInt(idColumnName))
             << ","
             << query.query.valueStr("name").simplified().toUpper().replace(ADJUST_NAME_REGEXP, "").left(25)
             << ","
             << QString::number(query.query.valueDouble("laty"), 'f', 8)
             << ","
             << QString::number(query.query.valueDouble("lonx"), 'f', 8)
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

    QueryWrapper query("select " + idColumnName + ", ident, region, tags, laty, lonx from " + tableName, db, ids, idColumnName);
    query.exec();
    while(query.next())
    {
      QString region = query.query.valueStr("region").toUpper();
      if(region.size() != 2)
        region = "ZZ";

      writer.writeStartElement("Waypoint");
      writer.writeAttribute("lat", QString::number(query.query.valueDouble("laty"), 'f', 8));
      writer.writeAttribute("lon", QString::number(query.query.valueDouble("lonx"), 'f', 8));
      writer.writeAttribute("waypointType", "NAMED");
      writer.writeAttribute("waypointRegion", region);
      writer.writeAttribute("magvar",
                            QString::number(magDec->getMagVar(Pos(query.query.valueFloat("lonx"),
                                                                  query.query.valueFloat("laty"))), 'f', 8));
      writer.writeAttribute("waypointIdent",
                            atools::fs::util::adjustIdent(query.query.valueStr("ident"), 5,
                                                          query.query.valueInt(idColumnName)));
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
