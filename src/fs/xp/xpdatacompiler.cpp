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

#include "fs/xp/xpdatacompiler.h"

#include "fs/navdatabaseoptions.h"
#include "fs/xp/xpfixwriter.h"
#include "fs/xp/xpnavwriter.h"
#include "fs/xp/xpairwaywriter.h"
#include "fs/xp/xpairportwriter.h"
#include "fs/xp/xpcifpwriter.h"
#include "fs/common/magdecreader.h"
#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "settings/settings.h"
#include "geo/pos.h"
#include "geo/calculations.h"
#include "fs/xp/airwaypostprocess.h"
#include "fs/progresshandler.h"
#include "exception.h"
#include "atools.h"
#include "fs/xp/xpairportindex.h"

#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QElapsedTimer>

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::buildPathNoCase;
using atools::settings::Settings;
using atools::fs::common::MagDecReader;

namespace atools {
namespace fs {
namespace xp {

// Reports per large file
const int NUM_REPORT_STEPS = 10000;

/* Report progress twice a second */
const int MIN_PROGRESS_REPORT_MS = 500;

XpDataCompiler::XpDataCompiler(sql::SqlDatabase& sqlDb, const NavDatabaseOptions& opts,
                               ProgressHandler *progressHandler)
  : options(opts), db(sqlDb), progress(progressHandler)
{
  basePath = buildBasePath(options);

  qInfo() << "Using X-Plane data path" << basePath;

  airportIndex = new XpAirportIndex();

  airportWriter = new XpAirportWriter(db, airportIndex, options, progress);
  fixWriter = new XpFixWriter(db, airportIndex, options, progress);
  navWriter = new XpNavWriter(db, airportIndex, options, progress);
  cifpWriter = new XpCifpWriter(db, airportIndex, options, progress);
  airwayWriter = new XpAirwayWriter(db, options, progress);
  airwayPostProcess = new AirwayPostProcess(db, options, progress);
  magDecReader = new MagDecReader();

  initQueries();
}

XpDataCompiler::~XpDataCompiler()
{
  close();
}

bool XpDataCompiler::writeBasepathScenery()
{
  writeSceneryArea(options.getBasepath());
  return false;
}

bool XpDataCompiler::compileEarthFix()
{
  QString path = buildPathNoCase({basePath, "earth_fix.dat"});
  return readDataFile(path, 5, fixWriter);
}

bool XpDataCompiler::compileEarthAirway()
{
  QString path = buildPathNoCase({basePath, "earth_awy.dat"});
  return readDataFile(path, 11, airwayWriter);
}

bool XpDataCompiler::postProcessEarthAirway()
{
  return airwayPostProcess->postProcessEarthAirway();
}

bool XpDataCompiler::compileEarthNav()
{
  QString path = buildPathNoCase({basePath, "earth_nav.dat"});
  return readDataFile(path, 11, navWriter);
}

bool XpDataCompiler::compileCustomApt()
{
  // X-Plane 11/Custom Scenery/KSEA Demo Area/Earth nav data/apt.dat
  // X-Plane 11/Custom Scenery/LFPG Paris - Charles de Gaulle/Earth Nav data/apt.dat
  QStringList localFindCustomAptDatFiles = findCustomAptDatFiles(options);
  for(const QString& aptdat : localFindCustomAptDatFiles)
  {
    if(readDataFile(aptdat, 1, airportWriter, IS_ADDON | READ_SHORT_REPORT))
      return true;
  }

  return false;
}

bool XpDataCompiler::compileCustomGlobalApt()
{
  // X-Plane 11/Custom Scenery/Global Airports/Earth nav data/apt.dat
  QString path = buildPathNoCase({options.getBasepath(),
                                  "Custom Scenery", "Global Airports", "Earth nav data", "apt.dat"});

  if(QFileInfo::exists(path))
    return readDataFile(path, 1, airportWriter);
  else
    // TODO report missing file
    qWarning() << path << "not found";

  return false;
}

bool XpDataCompiler::compileDefaultApt()
{
  // X-Plane 11/Resources/default scenery/default apt dat/Earth nav data/apt.dat
  QString defaultAptDat = buildPathNoCase({options.getBasepath(),
                                           "Resources", "default scenery", "default apt dat",
                                           "Earth nav data", "apt.dat"});

  if(QFileInfo::exists(defaultAptDat))
    return readDataFile(defaultAptDat, 1, airportWriter);
  else
    return false;
}

bool XpDataCompiler::compileCifp()
{
  QStringList cifpFiles = findCifpFiles(options);

  for(const QString& file : cifpFiles)
  {
    if(options.isIncludedFilename(file))
      if(readDataFile(file, 1, cifpWriter, READ_CIFP | READ_SHORT_REPORT))
        return true;
  }

  return false;
}

bool XpDataCompiler::compileLocalizers()
{

  QString path = buildPathNoCase({options.getBasepath(), "Custom Data", "Global Airports",
                                  "Earth nav data", "earth_nav.dat"});

  if(QFileInfo::exists(path))
    return readDataFile(path, 11, navWriter, READ_LOCALIZERS | READ_SHORT_REPORT);
  else
    // TODO report missing file
    qWarning() << path << "not found";

  return false;
}

bool XpDataCompiler::compileUserNav()
{
  QString path = buildPathNoCase({options.getBasepath(), "Custom Data", "user_nav.dat"});

  if(QFileInfo::exists(path))
    return readDataFile(path, 11, navWriter, READ_USER | READ_SHORT_REPORT);
  else
    return false;
}

bool XpDataCompiler::compileUserFix()
{
  QString path = buildPathNoCase({options.getBasepath(), "Custom Data", "user_fix.dat"});

  if(QFileInfo::exists(path))
    return readDataFile(path, 5, fixWriter, READ_USER | READ_SHORT_REPORT);
  else
    return false;
}

bool XpDataCompiler::compileMagDeclBgl()
{
  // Look first in config dir and then in local dir
  QString file = Settings::instance().getOverloadedPath(buildPath({QApplication::applicationDirPath(), "magdec", "magdec.bgl"}));

  qInfo() << "Reading" << file;

  magDecReader->readFromBgl(file);
  magDecReader->writeToTable(db);
  return false;
}

bool XpDataCompiler::readDataFile(const QString& filename, int minColumns, XpWriter *writer,
                                  atools::fs::xp::ContextFlags flags)
{
  QFile file;
  QTextStream stream;
  bool aborted = false;

  QString progressMsg = tr("Reading: %1").arg(filename);

  if(!options.isIncludedLocalPath(QFileInfo(filename).path()))
    return false;

  int lineNum = 1, totalNumLines, fileVersion = 0;

  // Open file and read header
  if(openFile(stream, file, filename, flags & READ_CIFP, lineNum, totalNumLines, fileVersion))
  {
    QFileInfo fi(filename);
    XpWriterContext context;
    context.curFileId = curFileId;
    context.fileName = fi.fileName();
    context.localPath = QDir(options.getBasepath()).relativeFilePath(fi.path());
    context.flags = flags | flagsFromOptions();
    context.fileVersion = fileVersion;
    context.magDecReader = magDecReader;

    if(flags & READ_SHORT_REPORT)
      if(progress->reportOther(progressMsg))
        return true;

    if(flags & READ_CIFP)
    {
      // Add additional information for procedure files
      context.cifpAirportIdent = QFileInfo(filename).baseName().toUpper();
      context.cifpAirportId = airportIndex->getAirportId(context.cifpAirportIdent).toInt();
    }

    QString line;
    QStringList fields;

    QElapsedTimer timer;
    timer.start();
    qint64 elapsed = timer.elapsed();

    int rowsPerStep =
      static_cast<int>(std::ceil(static_cast<float>(totalNumLines) / static_cast<float>(NUM_REPORT_STEPS)));
    int row = 0, steps = 0;

    // Read lines
    while(!stream.atEnd() && line != "99")
    {
      line = stream.readLine().trimmed();

      if(!(flags & READ_SHORT_REPORT))
      {
        if((row++ % rowsPerStep) == 0)
        {
          qint64 elapsed2 = timer.elapsed();

          // Update only every 500 ms - otherwise update only progress count
          bool silent = !(elapsed + MIN_PROGRESS_REPORT_MS < elapsed2);
          if(!silent)
            elapsed = elapsed2;

          steps++;
          if((aborted = progress->reportOther(progressMsg, -1, silent)) == true)
            break;
        }
      }

      if(!line.startsWith("#") && !line.isEmpty())
      {
        if(flags & READ_CIFP)
          fields = line.split(",");
        else
          fields = line.simplified().split(" ");

        if(fields.size() >= minColumns)
        {
          if(flags & READ_CIFP)
          {
            QString first = fields.takeFirst();
            QStringList rowCode = first.split(":");
            if(rowCode.size() == 2)
            {
              fields.prepend(rowCode.at(1));
              fields.prepend(rowCode.at(0));
            }
          }
          context.lineNumber = lineNum;

          // Call writer
          writer->write(fields, context);
        }
      }
      lineNum++;
    }
    if(!aborted)
      writer->finish(context);

    file.close();

    if(!(flags & READ_SHORT_REPORT))
      // Eat up any remaining progress steps
      progress->increaseCurrent(NUM_REPORT_STEPS - steps);
  }
  return aborted;
}

bool XpDataCompiler::openFile(QTextStream& stream, QFile& file, const QString& filename, bool cifpFormat, int& lineNum,
                              int& totalNumLines, int& fileVersion)
{
  file.setFileName(filename);
  lineNum = 1;

  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    writeFile(filename);

    stream.setDevice(&file);
    QString line;

    if(!cifpFormat)
    {
      // Byte order identifier
      line = stream.readLine();
      lineNum++;
      qInfo() << line;

      // Metadata
      line = stream.readLine();
      lineNum++;
      qInfo() << line;

      QStringList fields = line.simplified().split(" ");
      if(!fields.isEmpty())
        fileVersion = fields.first().toInt();

      if(!fields.isEmpty() && fileVersion < minVersion)
      {
        qWarning() << "Version of" << filename << "is" << fields.first() << "but expected a minimum of" << minVersion;
        return false;
      }

      qDebug() << "Counting lines for" << filename;
      qint64 pos = stream.pos();
      int lines = 0;
      while(!stream.atEnd())
      {
        stream.readLine();
        lines++;
      }
      totalNumLines = lines;
      stream.seek(pos);
      qDebug() << lines;
    }
  }
  else
    throw atools::Exception("Cannot open file " + filename + ". Reason: " + file.errorString());
  return true;
}

void XpDataCompiler::close()
{

  delete fixWriter;
  fixWriter = nullptr;

  delete navWriter;
  navWriter = nullptr;

  delete cifpWriter;
  cifpWriter = nullptr;

  delete airwayWriter;
  airwayWriter = nullptr;

  delete airportWriter;
  airportWriter = nullptr;

  delete airwayPostProcess;
  airwayPostProcess = nullptr;

  delete airportIndex;
  airportIndex = nullptr;

  delete magDecReader;
  magDecReader = nullptr;

  deInitQueries();
}

QStringList XpDataCompiler::findCustomAptDatFiles(const atools::fs::NavDatabaseOptions& opts)
{
  QStringList retval;

  // X-Plane 11/Custom Scenery/KSEA Demo Area/Earth nav data/apt.dat
  // X-Plane 11/Custom Scenery/LFPG Paris - Charles de Gaulle/Earth Nav data/apt.dat

  QDir customApt(buildPathNoCase({opts.getBasepath(), "Custom Scenery"}));

  QStringList dirs = customApt.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

  for(const QString& dir : dirs)
  {
    if(dir.toLower() == "global airports")
      continue;

    QFileInfo aptDat(buildPathNoCase({customApt.path(), dir, "Earth nav data", "apt.dat"}));

    if(aptDat.exists() && aptDat.isFile())
      retval.append(aptDat.filePath());
  }
  return retval;
}

QStringList XpDataCompiler::findCifpFiles(const atools::fs::NavDatabaseOptions& opts)
{
  // CIFP/$ICAO.dat
  QDir customDir(buildPathNoCase({opts.getBasepath(), "Custom Data", "CIFP"}));
  QDir defaultDir(buildPathNoCase({opts.getBasepath(), "Resources", "default data", "CIFP"}));

  QMap<QString, QFileInfo> entryMap;

  // Read all default entries
  QFileInfoList defaultEntries = defaultDir.entryInfoList({"*.dat"}, QDir::Files, QDir::NoSort);
  for(const QFileInfo& fileInfo : defaultEntries)
    entryMap.insert(fileInfo.fileName(), fileInfo);

  // Read custom entries and overwrite default
  QFileInfoList customEntries = customDir.entryInfoList({"*.dat"}, QDir::Files, QDir::NoSort);
  for(const QFileInfo& fileInfo : customEntries)
    entryMap.insert(fileInfo.fileName(), fileInfo);

  QStringList retval;
  for(const QFileInfo& fileInfo : entryMap.values())
    retval.append(fileInfo.filePath());
  return retval;
}

int XpDataCompiler::calculateReportCount(const atools::fs::NavDatabaseOptions& opts)
{
  int reportCount = 0;
  // Default or custom scenery files
  // earth_fix.dat earth_awy.dat earth_nav.dat
  reportCount += 3 * NUM_REPORT_STEPS;

  // X-Plane 11/Resources/default scenery/default apt dat/Earth nav data/apt.dat
  reportCount += NUM_REPORT_STEPS;

  // X-Plane 11/Custom Scenery/Global Airports/Earth nav data/apt.dat
  reportCount += NUM_REPORT_STEPS;

  // Default or custom CIFP/$ICAO.dat
  reportCount += findCifpFiles(opts).count();

  // X-Plane 11/Custom Scenery/KSEA Demo Area/Earth nav data/apt.dat
  // X-Plane 11/Custom Scenery/LFPG Paris - Charles de Gaulle/Earth Nav data/apt.dat
  reportCount += findCustomAptDatFiles(opts).count();

  // earth_nav.dat localizers $X-Plane/Custom Scenery/Global Airports/Earth nav data/
  if(QFileInfo::exists(buildPathNoCase({opts.getBasepath(),
                                        "Custom Scenery", "Global Airports", "Earth nav data", "earth_nav.dat"})))
    reportCount++;

  // user_nav.dat user_fix.dat $X-Plane/Custom Data/
  if(QFileInfo::exists(buildPathNoCase({opts.getBasepath(), "Custom Data", "user_nav.dat"})))
    reportCount++;
  if(QFileInfo::exists(buildPathNoCase({opts.getBasepath(), "Custom Data", "user_fix.dat"})))
    reportCount++;

  return reportCount;
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

  progress->incNumFiles();
}

void XpDataCompiler::writeSceneryArea(const QString& filepath)
{
  QFileInfo fileinfo(filepath);

  insertSceneryQuery->bindValue(":scenery_area_id", ++curSceneryId);
  insertSceneryQuery->bindValue(":number", curSceneryId);
  insertSceneryQuery->bindValue(":layer", curSceneryId);
  insertSceneryQuery->bindValue(":title", "X-Plane");
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
  QString customPath(buildPathNoCase({opts.getBasepath(), "Custom Data"}));
  QString defaultPath(buildPathNoCase({opts.getBasepath(), "Resources", "default data"}));

  if(QFileInfo::exists(buildPathNoCase({customPath, "earth_fix.dat"})) &&
     QFileInfo::exists(buildPathNoCase({customPath, "earth_awy.dat"})) &&
     QFileInfo::exists(buildPathNoCase({customPath, "earth_nav.dat"})))
    basePath = customPath;
  else if(QFileInfo::exists(buildPathNoCase({defaultPath, "earth_fix.dat"})) &&
          QFileInfo::exists(buildPathNoCase({defaultPath, "earth_awy.dat"})) &&
          QFileInfo::exists(buildPathNoCase({defaultPath, "earth_nav.dat"})))
    basePath = defaultPath;
  else
    throw atools::Exception("Cannot find valid files for X-Plane navdata");
  return basePath;
}

atools::fs::xp::ContextFlags XpDataCompiler::flagsFromOptions()
{
  ContextFlags flags = NO_FLAG;
  flags |= (options.isIncludedNavDbObject(atools::fs::type::ILS) ? INCLUDE_ILS : NO_FLAG);
  flags |= (options.isIncludedNavDbObject(atools::fs::type::VOR) ? INCLUDE_VOR : NO_FLAG);
  flags |= (options.isIncludedNavDbObject(atools::fs::type::NDB) ? INCLUDE_NDB : NO_FLAG);
  flags |= (options.isIncludedNavDbObject(atools::fs::type::MARKER) ? INCLUDE_MARKER : NO_FLAG);
  flags |= (options.isIncludedNavDbObject(atools::fs::type::AIRPORT) ? INCLUDE_AIRPORT : NO_FLAG);
  flags |= (options.isIncludedNavDbObject(atools::fs::type::APPROACH) ? INCLUDE_APPROACH : NO_FLAG);
  flags |= (options.isIncludedNavDbObject(atools::fs::type::APPROACHLEG) ? INCLUDE_APPROACHLEG : NO_FLAG);
  return flags;
}

} // namespace xp
} // namespace fs
} // namespace atools
