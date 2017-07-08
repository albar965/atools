/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include "fs/xp/xplanedatareader.h"

#include "fs/navdatabaseoptions.h"
#include "fs/xp/fixwriter.h"
#include "fs/xp/navwriter.h"
#include "fs/xp/airwaywriter.h"
#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "geo/pos.h"
#include "geo/calculations.h"
#include "fs/xp/airwaypostprocess.h"
#include "exception.h"

#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QDateTime>

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;

namespace atools {
namespace fs {
namespace xp {

XplaneDataCompiler::XplaneDataCompiler(sql::SqlDatabase& sqlDb, const NavDatabaseOptions& opts,
                                       ProgressHandler *progress)
  : options(opts), db(sqlDb), progressHandler(progress)
{
  basePath = buildBasePath(options);

  qInfo() << "Using X-Plane data path" << basePath;

  fixWriter = new FixWriter(db);
  navWriter = new NavWriter(db);
  airwayWriter = new AirwayWriter(db);
  airwayPostProcess = new AirwayPostProcess(db, options, progressHandler);

  initQueries();
}

XplaneDataCompiler::~XplaneDataCompiler()
{
  close();
}

bool XplaneDataCompiler::compileMeta()
{
  writeSceneryArea(basePath);
  return false;
}

bool XplaneDataCompiler::compileEarthFix()
{
  return readDatFile(basePath + QDir::separator() + "earth_fix.dat", 5, fixWriter);
}

bool XplaneDataCompiler::compileEarthAirway()
{
  return readDatFile(basePath + QDir::separator() + "earth_awy.dat", 11, airwayWriter);
}

bool XplaneDataCompiler::postProcessEarthAirway()
{
  return airwayPostProcess->postProcessEarthAirway();
}

bool XplaneDataCompiler::compileEarthNav()
{
  return readDatFile(basePath + QDir::separator() + "earth_nav.dat", 11, navWriter);
}

bool XplaneDataCompiler::compileApt()
{
  return false;
}

bool XplaneDataCompiler::readDatFile(const QString& filename, int minColumns, Writer *writer)
{
  QFile file;
  QTextStream stream;

  if(openFile(stream, file, filename))
  {
    QString line;

    while(!stream.atEnd() && line != "99")
    {
      line = stream.readLine();
      QStringList fields = line.simplified().split(" ");
      if(fields.size() >= minColumns)
        writer->write(fields, curFileId);
    }

    file.close();
    return false;
  }
  return true;
}

bool XplaneDataCompiler::openFile(QTextStream& stream, QFile& file, const QString& filename)
{
  file.setFileName(filename);

  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    qInfo() << "Opened" << filename;
    readFile(filename);

    stream.setDevice(&file);
    QString line;

    line = stream.readLine();
    qInfo() << line;
    line = stream.readLine();
    qInfo() << line;

    return true;
  }
  else
    throw atools::Exception("Cannot open file " + filename + ". Reason: " + file.errorString());
}

bool XplaneDataCompiler::writeCifp()
{
  return false;
}

bool XplaneDataCompiler::writeLocalizers()
{
  return false;
}

bool XplaneDataCompiler::writeUserNav()
{
  return false;
}

bool XplaneDataCompiler::writeUserFix()
{
  return false;
}

void XplaneDataCompiler::close()
{
  delete fixWriter;
  fixWriter = nullptr;

  delete navWriter;
  navWriter = nullptr;

  delete airwayWriter;
  airwayWriter = nullptr;

  delete airwayPostProcess;
  airwayPostProcess = nullptr;

  deInitQueries();
}

int XplaneDataCompiler::calculateFileCount(const atools::fs::NavDatabaseOptions& opts)
{
  int fileCount = 0;
  QString basePath = buildBasePath(opts);
  // Default or custom scenery files
  // earth_fix.dat earth_awy.dat earth_nav.dat
  fileCount += 3;

  // apt.dat
  fileCount += 1;

  // CIFP/$ICAO.dat
  QDir cifp(basePath + QDir::separator() + "CIFP");
  fileCount += cifp.entryList({"*.dat"}, QDir::Files, QDir::NoSort).count();

  // earth_nav.dat localizers $X-Plane/Custom Scenery/Global Airports/Earth nav data/
  if(QFileInfo::exists(opts.getBasepath() + QDir::separator() + "Custom Scenery" + QDir::separator() +
                       "Global Airports" + QDir::separator() + "Earth nav data" + QDir::separator() + "earth_nav.dat"))
    fileCount++;

  // user_nav.dat user_fix.dat $X-Plane/Custom Data/
  if(QFileInfo::exists(opts.getBasepath() + QDir::separator() + "user_nav.dat"))
    fileCount++;
  if(QFileInfo::exists(opts.getBasepath() + QDir::separator() + "user_fix.dat"))
    fileCount++;

  return fileCount;
}

void XplaneDataCompiler::readFile(const QString& filepath)
{
  QFileInfo fileinfo(filepath);

  insertFileQuery->bindValue(":bgl_file_id", ++curFileId);
  insertFileQuery->bindValue(":file_modification_time", fileinfo.lastModified().toTime_t());
  insertFileQuery->bindValue(":scenery_area_id", curSceneryId);
  insertFileQuery->bindValue(":bgl_create_time", fileinfo.lastModified().toTime_t());
  insertFileQuery->bindValue(":filepath", fileinfo.filePath());
  insertFileQuery->bindValue(":filename", fileinfo.fileName());
  insertFileQuery->bindValue(":size", fileinfo.size());
  insertFileQuery->exec();
}

void XplaneDataCompiler::writeSceneryArea(const QString& filepath)
{
  QFileInfo fileinfo(filepath);

  insertSceneryQuery->bindValue(":scenery_area_id", ++curSceneryId);
  insertSceneryQuery->bindValue(":number", curSceneryId);
  insertSceneryQuery->bindValue(":layer", curSceneryId);
  insertSceneryQuery->bindValue(":title", fileinfo.fileName());
  insertSceneryQuery->bindValue(":local_path", fileinfo.filePath());
  insertSceneryQuery->bindValue(":active", true);
  insertSceneryQuery->bindValue(":required", true);
  insertSceneryQuery->exec();
}

void XplaneDataCompiler::initQueries()
{
  deInitQueries();

  SqlUtil util(&db);

  insertSceneryQuery = new SqlQuery(db);
  insertSceneryQuery->prepare(util.buildInsertStatement("scenery_area", QString(), {"remote_path", "exclude"}));

  insertFileQuery = new SqlQuery(db);
  insertFileQuery->prepare(util.buildInsertStatement("bgl_file"));
}

void XplaneDataCompiler::deInitQueries()
{
  delete insertSceneryQuery;
  insertSceneryQuery = nullptr;

  delete insertFileQuery;
  insertFileQuery = nullptr;
}

QString XplaneDataCompiler::buildBasePath(const atools::fs::NavDatabaseOptions& opts)
{
  QString basePath;
  QString customPath(opts.getBasepath() + QDir::separator() + "Custom Data");
  QString defaultPath(opts.getBasepath() + QDir::separator() + "Resources" + QDir::separator() + "default data");

  if(QFileInfo::exists(customPath + QDir::separator() + "earth_fix.dat") &&
     QFileInfo::exists(customPath + QDir::separator() + "earth_awy.dat") &&
     QFileInfo::exists(customPath + QDir::separator() + "earth_nav.dat"))
    basePath = customPath;
  else if(QFileInfo::exists(defaultPath + QDir::separator() + "earth_fix.dat") &&
          QFileInfo::exists(defaultPath + QDir::separator() + "earth_awy.dat") &&
          QFileInfo::exists(defaultPath + QDir::separator() + "earth_nav.dat"))
    basePath = defaultPath;
  else
    throw atools::Exception("Cannot find valid files for X-Plane navdata");
  return basePath;
}

} // namespace xp
} // namespace fs
} // namespace atools
