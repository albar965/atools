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

#include "fs/xp/xpdatacompiler.h"

#include "fs/navdatabaseoptions.h"
#include "fs/xp/xpfixwriter.h"
#include "fs/xp/xpnavwriter.h"
#include "fs/xp/xpairwaywriter.h"
#include "fs/xp/xpairportwriter.h"
#include "fs/xp/xpcifpwriter.h"
#include "fs/xp/xpairspacewriter.h"
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
#include "fs/common/airportindex.h"
#include "fs/common/metadatawriter.h"
#include "fs/navdatabaseerrors.h"

#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextCodec>

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::buildPathNoCase;
using atools::settings::Settings;
using atools::fs::common::MagDecReader;
using atools::fs::common::MetadataWriter;
using atools::fs::common::AirportIndex;

namespace atools {
namespace fs {
namespace xp {

// Reports per large file
const static int NUM_REPORT_STEPS = 10000;

/* Report progress twice a second */
const static int MIN_PROGRESS_REPORT_MS = 500;

// Check for correct CIFP file basenames
const static QRegularExpression CIFP_MATCH("^[A-Z0-9]{3,8}$");

// 1100 Version - data cycle 1709, build 20170910, metadata AwyXP1101. Copyright (c) 2017 Navdata Provider
static QRegularExpression CYCLE_MATCH("data\\s+cycle\\s+([0-9]+)");

XpDataCompiler::XpDataCompiler(sql::SqlDatabase& sqlDb, const NavDatabaseOptions& opts,
                               ProgressHandler *progressHandler, NavDatabaseErrors *navdatabaseErrors)
  : options(opts), db(sqlDb), progress(progressHandler), errors(navdatabaseErrors)
{
  basePath = buildBasePath(options);

  qInfo() << "Using X-Plane data path" << basePath;

  airportIndex = new AirportIndex();

  airportWriter = new XpAirportWriter(db, airportIndex, options, progress, errors);
  fixWriter = new XpFixWriter(db, airportIndex, options, progress, errors);
  navWriter = new XpNavWriter(db, airportIndex, options, progress, errors);
  cifpWriter = new XpCifpWriter(db, airportIndex, options, progress, errors);
  airspaceWriter = new XpAirspaceWriter(db, options, progress, errors);
  airwayWriter = new XpAirwayWriter(db, options, progress, errors);
  airwayPostProcess = new AirwayPostProcess(db, options, progress);
  metadataWriter = new MetadataWriter(db);
  magDecReader = new MagDecReader();

  initQueries();
}

XpDataCompiler::~XpDataCompiler()
{
  close();
}

bool XpDataCompiler::writeBasepathScenery()
{
  metadataWriter->writeSceneryArea(options.getBasepath(), "X-Plane", ++curSceneryId);
  db.commit();
  return false;
}

bool XpDataCompiler::compileEarthFix()
{
  QString path = buildPathNoCase({basePath, "earth_fix.dat"});

  if(QFileInfo::exists(path))
  {
    bool aborted = readDataFile(path, 5, fixWriter, UPDATE_CYCLE);

    if(!aborted)
      db.commit();
    return aborted;
  }
  else
    throw Exception(tr("Default file \"%1\" not found").arg(path));
}

bool XpDataCompiler::compileEarthAirway()
{

  QString path = buildPathNoCase({basePath, "earth_awy.dat"});

  if(QFileInfo::exists(path))
  {

    bool aborted = readDataFile(path, 11, airwayWriter, UPDATE_CYCLE);
    if(!aborted)
      db.commit();
    return aborted;
  }
  else
    throw Exception(tr("Default file \"%1\" not found").arg(path));
}

bool XpDataCompiler::postProcessEarthAirway()
{
  bool aborted = false;
  if((aborted = progress->reportOther(tr("Post procecssing Airways"))))
    return true;

  aborted = airwayPostProcess->postProcessEarthAirway();
  if(!aborted)
    db.commit();
  return aborted;
}

bool XpDataCompiler::compileEarthNav()
{
  QString path = buildPathNoCase({basePath, "earth_nav.dat"});
  if(QFileInfo::exists(path))
  {
    bool aborted = readDataFile(path, 11, navWriter, UPDATE_CYCLE);
    if(!aborted)
      db.commit();
    return aborted;
  }
  else
    throw Exception(tr("Default file \"%1\" not found").arg(path));
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
  db.commit();
  return false;
}

bool XpDataCompiler::compileCustomGlobalApt()
{
  // X-Plane 11/Custom Scenery/Global Airports/Earth nav data/apt.dat
  QString path = buildPathNoCase({options.getBasepath(),
                                  "Custom Scenery", "Global Airports", "Earth nav data", "apt.dat"});

  if(QFileInfo::exists(path))
  {
    bool aborted = readDataFile(path, 1, airportWriter, IS_3D);
    if(!aborted)
      db.commit();
    return aborted;
  }
  else
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
  {
    bool aborted = readDataFile(defaultAptDat, 1, airportWriter);
    if(!aborted)
      db.commit();
    return aborted;
  }
  else
    throw Exception(tr("Default file \"%1\" not found").arg(defaultAptDat));
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
  db.commit();

  return false;
}

bool XpDataCompiler::compileAirspaces()
{
  QStringList airspaceFiles = findAirspaceFiles(options);

  for(const QString& file : airspaceFiles)
  {
    if(options.isIncludedFilename(file))
      if(readDataFile(file, 1, airspaceWriter, READ_AIRSPACE | READ_SHORT_REPORT))
        return true;
  }
  db.commit();

  return false;
}

bool XpDataCompiler::compileLocalizers()
{
  QString path = buildPathNoCase({options.getBasepath(), "Custom Scenery", "Global Airports",
                                  "Earth nav data", "earth_nav.dat"});

  if(QFileInfo::exists(path))
  {
    bool aborted = readDataFile(path, 11, navWriter, READ_LOCALIZERS | READ_SHORT_REPORT);
    if(!aborted)
      db.commit();
    return aborted;
  }
  else
    // TODO report missing file
    qWarning() << path << "not found";

  return false;
}

bool XpDataCompiler::compileUserNav()
{
  QString path = buildPathNoCase({options.getBasepath(), "Custom Data", "user_nav.dat"});

  if(QFileInfo::exists(path))
  {
    bool aborted = readDataFile(path, 11, navWriter, READ_USER | READ_SHORT_REPORT);
    if(!aborted)
      db.commit();
    return aborted;
  }
  else
    return false;
}

bool XpDataCompiler::compileUserFix()
{
  QString path = buildPathNoCase({options.getBasepath(), "Custom Data", "user_fix.dat"});

  if(QFileInfo::exists(path))
  {
    bool aborted = readDataFile(path, 5, fixWriter, READ_USER | READ_SHORT_REPORT);
    if(!aborted)
      db.commit();
    return aborted;
  }
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
  db.commit();
  return false;
}

bool XpDataCompiler::readDataFile(const QString& filepath, int minColumns, XpWriter *writer,
                                  atools::fs::xp::ContextFlags flags)
{
  QFile file;
  QTextStream stream;
  bool aborted = false;

  QString progressMsg = tr("Reading: %1").arg(filepath);
  QFileInfo fileinfo(filepath);

  if(!includeFile(fileinfo))
    return false;

  if(!options.isAddonDirectory(fileinfo.absolutePath()))
    // Clear add-on flag if directory is excluded
    flags &= ~atools::fs::xp::IS_ADDON;

  int lineNum = 1, totalNumLines, fileVersion = 0;

  try
  {
    // Open file and read header
    if(openFile(stream, file, filepath, flags, lineNum, totalNumLines, fileVersion))
    {
      XpWriterContext context;
      context.curFileId = curFileId;
      context.fileName = fileinfo.fileName();
      context.filePath = fileinfo.filePath();
      context.localPath = QDir(options.getBasepath()).relativeFilePath(fileinfo.path());
      context.flags = flags | flagsFromOptions();
      context.fileVersion = fileVersion;
      context.magDecReader = magDecReader;

      if(flags & READ_SHORT_REPORT)
        if(progress->reportOther(progressMsg))
          return true;

      if(flags & READ_CIFP)
      {
        QString ident = QFileInfo(filepath).baseName().toUpper();
        if(!CIFP_MATCH.match(ident).hasMatch())
          throw atools::Exception("CIFP file has no valid name which should match airport ident.");

        // Add additional information for procedure files
        context.cifpAirportIdent = QFileInfo(filepath).baseName().toUpper();
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

        if(flags & READ_AIRSPACE && !line.startsWith("AN"))
        {
          // Strip OpenAirport file comments except for airport names
          int idx = line.indexOf("*");
          if(idx != -1)
            line = line.left(line.indexOf("*"));
        }
        else if(!(flags & READ_CIFP))
        {
          // Strip dat-file comments
          if(line.startsWith("#"))
            line.clear();
        }

        if(!line.isEmpty())
        {
          if(flags & READ_CIFP)
            fields = line.split(",");
          else
            fields = line.simplified().split(" ");

          if(fields.size() >= minColumns)
          {
            if(flags & READ_CIFP)
            {
              // Extract colon separated row code
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
  }
  catch(std::exception& e)
  {
    if(errors != nullptr)
    {
      progress->reportError();
      errors->sceneryErrors.first().fileErrors.append({fileinfo.filePath(), e.what(), lineNum});
      qWarning() << Q_FUNC_INFO << "Error in file" << fileinfo.filePath() << "line" << lineNum << ":" << e.what();
    }
    else
    {
      writer->reset();
      // Enrich error message and rethrow a new one
      throw atools::Exception(QString("Caught exception in file \"%1\" in line %2. Message: %3").
                              arg(fileinfo.filePath()).arg(lineNum).arg(e.what()));
    }
  }
  writer->reset();
  return aborted;
}

bool XpDataCompiler::openFile(QTextStream& stream, QFile& filepath, const QString& filename, atools::fs::xp::ContextFlags flags,
                              int& lineNum, int& totalNumLines, int& fileVersion)
{
  filepath.setFileName(filename);
  lineNum = 1;

  if(filepath.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    if(flags & READ_AIRSPACE)
    {
      // Try to detect code using the BOM for airspaces only - use ANSI as fallback
      stream.setDevice(&filepath);
      stream.setCodec(atools::codecForFile(filepath, QTextCodec::codecForName("Windows-1252")));
    }
    else
    {
      stream.setDevice(&filepath);
      stream.setCodec("UTF-8");
    }
    stream.setAutoDetectUnicode(true);

    QString line;

    if(!(flags & READ_CIFP) && !(flags & READ_AIRSPACE))
    {
      // Read file header =============================
      // Byte order identifier
      line = stream.readLine();
      lineNum++;
      qInfo() << line;

      // Metadata and copyright
      line = stream.readLine();
      lineNum++;
      qInfo() << line;

      QStringList fields = line.simplified().split(" ");
      if(!fields.isEmpty())
        fileVersion = fields.first().toInt();

      if(!fields.isEmpty() && fileVersion < minFileVersion)
      {
        qWarning() << "Version of" << filename << "is" << fields.first() << "but expected a minimum of" << minFileVersion;
        throw atools::Exception(QString("Found file version %1. Minimum supported is %2.").
                                arg(fields.first()).arg(minFileVersion));
      }

      metadataWriter->writeFile(filename, QString(), curSceneryId, ++curFileId);
      progress->incNumFiles();

      if(flags & UPDATE_CYCLE)
        updateAiracCycleFromHeader(line, filename, lineNum);

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
    else
    {
      metadataWriter->writeFile(filename, QString(), curSceneryId, ++curFileId);
      progress->incNumFiles();
    }
  }
  else
    throw atools::Exception("Cannot open file. Reason: " + filepath.errorString() + ".");
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

  delete airspaceWriter;
  airspaceWriter = nullptr;

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

  delete metadataWriter;
  metadataWriter = nullptr;

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

    if(!includeFile(opts, aptDat))
      continue;

    if(aptDat.exists() && aptDat.isFile())
      retval.append(aptDat.filePath());
  }
  return retval;
}

QStringList XpDataCompiler::findAirspaceFiles(const atools::fs::NavDatabaseOptions& opts)
{
  QString additionalDir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first() +
                          QDir::separator() + "Little Navmap" + QDir::separator() + "X-Plane Airspaces";
  if(!QFile::exists(additionalDir))
    additionalDir.clear();

  return findFiles(opts, "airspaces", {"*.txt"}, additionalDir, false);
}

QStringList XpDataCompiler::findCifpFiles(const atools::fs::NavDatabaseOptions& opts)
{
  return findFiles(opts, "CIFP", {"*.dat"}, QString(), true);
}

QStringList XpDataCompiler::findFiles(const atools::fs::NavDatabaseOptions& opts, const QString& subdir,
                                      const QStringList& pattern, const QString& additionalDir, bool makeUnique)
{
  QDir customDir(buildPathNoCase({opts.getBasepath(), "Custom Data", subdir}));
  QDir defaultDir(buildPathNoCase({opts.getBasepath(), "Resources", "default data", subdir}));

  QMap<QString, QFileInfo> entryMap;

  // Read all default entries
  QFileInfoList defaultEntries = defaultDir.entryInfoList(pattern, QDir::Files, QDir::NoSort);
  for(const QFileInfo& fileInfo : defaultEntries)
  {
    if(includeFile(opts, fileInfo))
      entryMap.insert(makeUnique ? fileInfo.fileName().toUpper() : fileInfo.filePath(), fileInfo);
  }

  // Read custom entries and overwrite default
  QFileInfoList customEntries = customDir.entryInfoList(pattern, QDir::Files, QDir::NoSort);
  for(const QFileInfo& fileInfo : customEntries)
  {
    if(includeFile(opts, fileInfo))
      entryMap.insert(makeUnique ? fileInfo.fileName().toUpper() : fileInfo.filePath(), fileInfo);
  }

  if(!additionalDir.isEmpty())
  {
    // Read the additional directory
    QDir ad(additionalDir);
    QFileInfoList additonalEntries = ad.entryInfoList(pattern, QDir::Files, QDir::NoSort);
    for(const QFileInfo& fileInfo : additonalEntries)
    {
      if(includeFile(opts, fileInfo))
        entryMap.insert(makeUnique ? fileInfo.fileName().toUpper() : fileInfo.filePath(), fileInfo);
    }
  }

  QStringList retval;
  for(const QFileInfo& fileInfo : entryMap.values())
    retval.append(fileInfo.filePath());

  // qDebug() << "== Files ==============================================================================";
  // qDebug() << Q_FUNC_INFO << retval;
  // qDebug() << "================================================================================";

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

  reportCount += findAirspaceFiles(opts).count();

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

void XpDataCompiler::updateAiracCycleFromHeader(const QString& header, const QString& filepath, int lineNum)
{
  // 1100 Version - data cycle 1709, build 20170910, metadata AwyXP1101. Copyright (c) 2017 Navdata Provider
  QRegularExpressionMatch match = CYCLE_MATCH.match(header);
  if(match.hasMatch())
  {
    QString c = match.captured(1);
    if(c.isEmpty())
    {
      progress->reportError();
      qWarning() << Q_FUNC_INFO << "Error in file" << filepath << "line" << lineNum << ": AIRAC cycle in file is empty.";
      errors->sceneryErrors.first().fileErrors.append({filepath, tr("AIRAC cycle in file is empty."), lineNum});
    }
    else if(airacCycle.isEmpty())
      airacCycle = c;
    else if(airacCycle != c)
    {
      progress->reportError();

      QString msg = tr("Found different AIRAC cycles across navdata files. "
                       "%1 and %2").arg(airacCycle).arg(c);
      qWarning() << Q_FUNC_INFO << "Error in file" << filepath << "line" << lineNum << ": " << msg;
      errors->sceneryErrors.first().fileErrors.append({filepath, msg, lineNum});
    }
  }
  else
  {
    qWarning() << Q_FUNC_INFO << "Error in file" << filepath << "line" << lineNum << ": AIRAC cycle not found in file.";
    progress->reportError();
    errors->sceneryErrors.first().fileErrors.append({filepath, tr("AIRAC cycle not found in file."), lineNum});
  }
}

void XpDataCompiler::initQueries()
{
  deInitQueries();
  if(metadataWriter != nullptr)
    metadataWriter->initQueries();
}

void XpDataCompiler::deInitQueries()
{
  if(metadataWriter != nullptr)
    metadataWriter->deInitQueries();
}

QString XpDataCompiler::buildBasePath(const atools::fs::NavDatabaseOptions& opts)
{
  QString basePath;
  QString customPath(buildPathNoCase({opts.getBasepath(), "Custom Data"}));
  QString defaultPath(buildPathNoCase({opts.getBasepath(), "Resources", "default data"}));

  if(includeFile(opts, customPath) &&
     QFileInfo::exists(buildPathNoCase({customPath, "earth_fix.dat"})) &&
     QFileInfo::exists(buildPathNoCase({customPath, "earth_awy.dat"})) &&
     QFileInfo::exists(buildPathNoCase({customPath, "earth_nav.dat"})))
    basePath = customPath;
  else if(QFileInfo::exists(buildPathNoCase({defaultPath, "earth_fix.dat"})) &&
          QFileInfo::exists(buildPathNoCase({defaultPath, "earth_awy.dat"})) &&
          QFileInfo::exists(buildPathNoCase({defaultPath, "earth_nav.dat"})))
    basePath = defaultPath;
  else
    throw atools::Exception(tr("Cannot find valid files for X-Plane navdata in either\n\"%1\" or\n\"%2\"\n\n"
                               "Make sure that earth_fix.dat, earth_awy.dat and earth_nav.dat "
                               "can be found in on of these paths.").
                            arg(customPath).arg(defaultPath));
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

bool XpDataCompiler::includeFile(const QFileInfo& fileinfo)
{
  return includeFile(options, fileinfo);
}

bool XpDataCompiler::includeFile(const NavDatabaseOptions& opts, const QFileInfo& fileinfo)
{
  if(!opts.isIncludedLocalPath(fileinfo.path()))
    // Excluded in configuration file
    return false;

  // Excluded in the GUI
  if(fileinfo.isDir())
  {
    if(!opts.isIncludedDirectory(fileinfo.absoluteFilePath()))
      return false;
  }
  else if(fileinfo.isFile())
  {
    if(!opts.isIncludedDirectory(fileinfo.absolutePath()))
      return false;
  }

  return true;
}

} // namespace xp
} // namespace fs
} // namespace atools
