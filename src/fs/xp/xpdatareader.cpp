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

#include "fs/xp/xpdatareader.h"

#include "fs/navdatabaseoptions.h"
#include "fs/xp/xpfixwriter.h"
#include "fs/xp/xpnavwriter.h"
#include "fs/xp/xpairwaywriter.h"
#include "fs/xp/xpairportwriter.h"
#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "geo/pos.h"
#include "geo/calculations.h"
#include "fs/xp/airwaypostprocess.h"
#include "fs/progresshandler.h"
#include "exception.h"
#include "atools.h"

#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QDateTime>

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;

namespace atools {
namespace fs {
namespace xp {

XpDataCompiler::XpDataCompiler(sql::SqlDatabase& sqlDb, const NavDatabaseOptions& opts,
                               ProgressHandler *progress)
  : options(opts), db(sqlDb), progressHandler(progress)
{
  basePath = buildBasePath(options);

  qInfo() << "Using X-Plane data path" << basePath;

  fixWriter = new XpFixWriter(db);
  navWriter = new XpNavWriter(db);
  airwayWriter = new XpAirwayWriter(db);
  airportWriter = new XpAirportWriter(db);
  airwayPostProcess = new AirwayPostProcess(db, options, progressHandler);

  initQueries();
}

XpDataCompiler::~XpDataCompiler()
{
  close();
}

bool XpDataCompiler::writeBasepathScenery()
{
  writeSceneryArea(basePath);
  return false;
}

bool XpDataCompiler::compileEarthFix()
{
  QString path = atools::buildPathNoCase({basePath, "earth_fix.dat"});

  if(progressHandler->reportOther(tr("Reading: %1").arg(path)) == true)
    return true;

  return readDataFile(path, 5, fixWriter);
}

bool XpDataCompiler::compileEarthAirway()
{
  QString path = atools::buildPathNoCase({basePath, "earth_awy.dat"});
  if(progressHandler->reportOther(tr("Reading: %1").arg(path)) == true)
    return true;

  return readDataFile(path, 11, airwayWriter);
}

bool XpDataCompiler::postProcessEarthAirway()
{
  return airwayPostProcess->postProcessEarthAirway();
}

bool XpDataCompiler::compileEarthNav()
{
  QString path = atools::buildPathNoCase({basePath, "earth_nav.dat"});
  if(progressHandler->reportOther(tr("Reading: %1").arg(path)) == true)
    return true;

  return readDataFile(path, 11, navWriter);
}

bool XpDataCompiler::compileCustomApt()
{
  bool aborted = false;
  // X-Plane 11/Custom Scenery/KSEA Demo Area/Earth nav data/apt.dat
  // X-Plane 11/Custom Scenery/LFPG Paris - Charles de Gaulle/Earth Nav data/apt.dat
  QStringList localFindCustomAptDatFiles = findCustomAptDatFiles(options);
  for(const QString& aptdat : localFindCustomAptDatFiles)
  {
    if((aborted = progressHandler->reportOther(tr("Reading: %1").arg(aptdat))) == true)
      return aborted;

    if((aborted = readDataFile(aptdat, 1, airportWriter)) == true)
      return aborted;
  }

  return false;
}

bool XpDataCompiler::compileCustomGlobalApt()
{
  // X-Plane 11/Custom Scenery/Global Airports/Earth nav data/apt.dat
  QString customAptDat = atools::buildPathNoCase({options.getBasepath(),
                                                  "Custom Scenery", "Global Airports", "Earth nav data", "apt.dat"});

  if(QFileInfo::exists(customAptDat))
  {
    if(progressHandler->reportOther(tr("Reading: %1").arg(customAptDat)) == true)
      return true;

    return readDataFile(customAptDat, 1, airportWriter);
  }

  return false;
}

bool XpDataCompiler::compileDefaultApt()
{
  // X-Plane 11/Resources/default scenery/default apt dat/Earth nav data/apt.dat
  QString defaultAptDat = atools::buildPathNoCase({options.getBasepath(),
                                                   "Resources", "default scenery", "default apt dat",
                                                   "Earth nav data", "apt.dat"});

  if(QFileInfo::exists(defaultAptDat))
  {
    if(progressHandler->reportOther(tr("Reading: %1").arg(defaultAptDat)) == true)
      return true;

    return readDataFile(defaultAptDat, 1, airportWriter);
  }
  return false;
}

bool XpDataCompiler::readDataFile(const QString& filename, int minColumns, XpWriter *writer)
{
  QFile file;
  QTextStream stream;

  if(openFile(stream, file, filename))
  {
    QString line;

    int lineNum = 0;
    while(!stream.atEnd() && line != "99")
    {
      line = stream.readLine().trimmed();

      if((lineNum % 500) == 0)
        progressHandler->reportUpdate();

      if(!line.startsWith("#") && !line.isEmpty())
      {
        QStringList fields = line.simplified().split(" ");
        if(fields.size() >= minColumns)
          writer->write(fields, curFileId);
      }
      lineNum++;
    }
    writer->finish();

    file.close();
    return false;
  }
  return true;
}

bool XpDataCompiler::openFile(QTextStream& stream, QFile& file, const QString& filename)
{
  file.setFileName(filename);

  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    qInfo() << "Opened" << filename;
    writeFile(filename);

    stream.setDevice(&file);
    QString line;

    // Byte order identifier
    line = stream.readLine();
    qInfo() << line;

    // Metadata
    line = stream.readLine();
    qInfo() << line;

    QStringList fields = line.simplified().split(" ");
    if(!fields.isEmpty() && fields.first().toInt() < minVersion)
      throw Exception(tr("Version is %1 but expected a minimum of %2").arg(fields.first()).arg(minVersion));

    return true;
  }
  else
    throw atools::Exception("Cannot open file " + filename + ". Reason: " + file.errorString());
}

bool XpDataCompiler::writeCifp()
{

  QStringList cifpFiles = findCifpFiles(options);

  for(const QString& file : cifpFiles)
  {
    if(progressHandler->reportOther(tr("Reading: %1").arg(file)) == true)
      return true;
  }

  return false;
}

bool XpDataCompiler::writeLocalizers()
{
  if(progressHandler->reportOther("Localizers TODO") == true)
    return true;

  return false;
}

bool XpDataCompiler::writeUserNav()
{
  if(progressHandler->reportOther("User Nav TODO") == true)
    return true;

  return false;
}

bool XpDataCompiler::writeUserFix()
{
  if(progressHandler->reportOther("User Fix TODO") == true)
    return true;

  return false;
}

void XpDataCompiler::close()
{
  delete fixWriter;
  fixWriter = nullptr;

  delete navWriter;
  navWriter = nullptr;

  delete airwayWriter;
  airwayWriter = nullptr;

  delete airportWriter;
  airportWriter = nullptr;

  delete airwayPostProcess;
  airwayPostProcess = nullptr;

  deInitQueries();
}

QStringList XpDataCompiler::findCustomAptDatFiles(const atools::fs::NavDatabaseOptions& opts)
{
  QStringList retval;

  // X-Plane 11/Custom Scenery/KSEA Demo Area/Earth nav data/apt.dat
  // X-Plane 11/Custom Scenery/LFPG Paris - Charles de Gaulle/Earth Nav data/apt.dat

  QDir customApt(atools::buildPathNoCase({opts.getBasepath(), "Custom Scenery"}));

  QStringList dirs = customApt.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);

  for(const QString& dir : dirs)
  {
    if(dir.toLower() == "global airports")
      continue;

    QFileInfo aptDat(atools::buildPathNoCase({customApt.path(), dir, "Earth nav data", "apt.dat"}));

    if(aptDat.exists() && aptDat.isFile())
      retval.append(aptDat.filePath());
  }
  return retval;
}

QStringList XpDataCompiler::findCifpFiles(const atools::fs::NavDatabaseOptions& opts)
{
  QString basePath = buildBasePath(opts);

  // CIFP/$ICAO.dat
  QDir cifp(basePath + QDir::separator() + "CIFP");
  return cifp.entryList({"*.dat"}, QDir::Files, QDir::NoSort);
}

int XpDataCompiler::calculateFileCount(const atools::fs::NavDatabaseOptions& opts)
{
  int fileCount = 0;
  // Default or custom scenery files
  // earth_fix.dat earth_awy.dat earth_nav.dat
  fileCount += 3;

  // apt.dat
  fileCount += 1;

  fileCount += findCifpFiles(opts).count();
  fileCount += findCustomAptDatFiles(opts).count();

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

void XpDataCompiler::writeFile(const QString& filepath)
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

void XpDataCompiler::writeSceneryArea(const QString& filepath)
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

void XpDataCompiler::initQueries()
{
  deInitQueries();

  SqlUtil util(&db);

  insertSceneryQuery = new SqlQuery(db);
  insertSceneryQuery->prepare(util.buildInsertStatement("scenery_area", QString(), {"remote_path", "exclude"}));

  insertFileQuery = new SqlQuery(db);
  insertFileQuery->prepare(util.buildInsertStatement("bgl_file"));
}

void XpDataCompiler::deInitQueries()
{
  delete insertSceneryQuery;
  insertSceneryQuery = nullptr;

  delete insertFileQuery;
  insertFileQuery = nullptr;
}

QString XpDataCompiler::buildBasePath(const atools::fs::NavDatabaseOptions& opts)
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
