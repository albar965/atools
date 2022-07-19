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

#include "fs/xp/xpdatacompiler.h"

#include "fs/navdatabaseoptions.h"
#include "fs/xp/xpfixwriter.h"
#include "fs/xp/xpmorawriter.h"
#include "fs/xp/xpairportmsawriter.h"
#include "fs/xp/xpholdingwriter.h"
#include "fs/xp/xpnavwriter.h"
#include "fs/xp/xpairwaywriter.h"
#include "fs/xp/xpairportwriter.h"
#include "fs/xp/xpcifpwriter.h"
#include "fs/xp/xpairspacewriter.h"
#include "fs/xp/scenerypacks.h"
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
using atools::checkFile;
using atools::settings::Settings;
using atools::fs::common::MagDecReader;
using atools::fs::common::MetadataWriter;
using atools::fs::common::AirportIndex;
using atools::fs::xp::SceneryPack;
using atools::fs::xp::SceneryPacks;

namespace atools {
namespace fs {
namespace xp {

// Reports per large file
const static int NUM_REPORT_STEPS = 1000;
// Reports per smaller file
const static int NUM_REPORT_STEPS_SMALL = 100;

const static int NUM_REPORT_STEPS_CIFP = 2000;

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
  // Build base for earth_fix.dat earth_awy.dat and earth_nav.dat - these files have to match and exist in the same folder
  basePath = buildBasePath(options, QString());

  qInfo() << Q_FUNC_INFO << "Using X-Plane data path" << basePath;

  airportIndex = new AirportIndex();

  airportWriter = new XpAirportWriter(db, airportIndex, options, progress, errors);
  moraWriter = new XpMoraWriter(db, options, progress, errors);
  airportMsaWriter = new XpAirportMsaWriter(db, airportIndex, options, progress, errors);
  holdingWriter = new XpHoldingWriter(db, airportIndex, options, progress, errors);
  fixWriter = new XpFixWriter(db, airportIndex, options, progress, errors);
  navWriter = new XpNavWriter(db, airportIndex, options, progress, errors);
  cifpWriter = new XpCifpWriter(db, airportIndex, options, progress, errors);
  airspaceWriter = new XpAirspaceWriter(db, options, progress, errors);
  airwayWriter = new XpAirwayWriter(db, options, progress, errors);
  airwayPostProcess = new AirwayPostProcess(db);
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

  if(checkFile(path))
  {
    bool aborted = readDataFile(path, 5, fixWriter, UPDATE_CYCLE, NUM_REPORT_STEPS_SMALL);

    if(!aborted)
      db.commit();
    return aborted;
  }
  else
    throw Exception(tr("Default file \"%1\" not found").arg(path));
}

bool XpDataCompiler::compileEarthMora()
{
  // Check Resources and Custom Data for file
  QString filepath = buildBasePath(options, "earth_mora.dat");

  if(checkFile(filepath))
  {
    // NO_FLAG - ignore AIRAC discrepancies
    bool aborted = readDataFile(filepath, 5, moraWriter, NO_FLAG, NUM_REPORT_STEPS_SMALL);

    if(!aborted)
      db.commit();
    return aborted;
  }

  return false;
}

bool XpDataCompiler::compileEarthAirportMsa()
{
  // Check Resources and Custom Data for file
  QString filepath = buildBasePath(options, "earth_msa.dat");

  if(checkFile(filepath))
  {
    // NO_FLAG - ignore AIRAC discrepancies
    bool aborted = readDataFile(filepath, 5, airportMsaWriter, NO_FLAG, NUM_REPORT_STEPS_SMALL);

    if(!aborted)
      db.commit();
    return aborted;
  }

  return false;
}

bool XpDataCompiler::compileEarthHolding()
{
  // Check Resources and Custom Data for file
  QString filepath = buildBasePath(options, "earth_hold.dat");

  if(checkFile(filepath))
  {
    // NO_FLAG - ignore AIRAC discrepancies
    bool aborted = readDataFile(filepath, 5, holdingWriter, NO_FLAG, NUM_REPORT_STEPS_SMALL);

    if(!aborted)
      db.commit();
    return aborted;
  }

  return false;
}

bool XpDataCompiler::compileEarthAirway()
{
  QString path = buildPathNoCase({basePath, "earth_awy.dat"});

  if(checkFile(path))
  {

    bool aborted = readDataFile(path, 11, airwayWriter, UPDATE_CYCLE, NUM_REPORT_STEPS_SMALL);
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
  if(checkFile(path))
  {
    bool aborted = readDataFile(path, 11, navWriter, UPDATE_CYCLE, NUM_REPORT_STEPS_SMALL);
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
  QStringList localFindCustomAptDatFiles = findCustomAptDatFiles(options, errors, progress);
  for(const QString& aptdat : localFindCustomAptDatFiles)
  {
    // Only one progress report per file
    if(readDataFile(aptdat, 1, airportWriter, IS_ADDON | READ_SHORT_REPORT, 1))
      return true;
  }
  db.commit();
  return false;
}

bool XpDataCompiler::compileCustomGlobalApt()
{
  // X-Plane 11/Custom Scenery/Global Airports/Earth nav data/apt.dat
  QString path = buildPathNoCase({options.getBasepath(), "Custom Scenery", "Global Airports", "Earth nav data", "apt.dat"});

  if(checkFile(path))
  {
    bool aborted = readDataFile(path, 1, airportWriter, xp::NO_FLAG, NUM_REPORT_STEPS);
    if(!aborted)
      db.commit();
    return aborted;
  }

  return false;
}

bool XpDataCompiler::compileGlobalApt12()
{
  // X-Plane 12/Global Scenery/Global Airports/Earth nav data/apt.dat
  QString path = buildPathNoCase({options.getBasepath(), "Global Scenery", "Global Airports", "Earth nav data", "apt.dat"});

  if(checkFile(path))
  {
    bool aborted = readDataFile(path, 1, airportWriter, xp::NO_FLAG, NUM_REPORT_STEPS);
    if(!aborted)
      db.commit();
    return aborted;
  }

  return false;
}

bool XpDataCompiler::compileDefaultApt()
{
  // X-Plane 11/Resources/default scenery/default apt dat/Earth nav data/apt.dat (330 MB)
  QString defaultAptDat = buildPathNoCase(
    {options.getBasepath(), "Resources", "default scenery", "default apt dat", "Earth nav data", "apt.dat"});

  if(checkFile(defaultAptDat))
  {
    bool aborted = readDataFile(defaultAptDat, 1, airportWriter, xp::NO_FLAG, NUM_REPORT_STEPS);
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

  int rowsPerStep =
    static_cast<int>(std::ceil(static_cast<float>(cifpFiles.size()) / static_cast<float>(NUM_REPORT_STEPS_CIFP)));
  int row = 0, steps = 0;

  for(const QString& file : cifpFiles)
  {
    if(options.isIncludedFilename(file))
    {
      if(readDataFile(file, 1, cifpWriter, READ_CIFP | READ_SHORT_REPORT, 0))
        return true;

      if((row % rowsPerStep) == 0)
      {
        progress->reportOther(tr("Reading: %1").arg(file));
        steps++;
      }
      row++;
    }
  }

  // Consume remaining progress steps
  progress->increaseCurrent(NUM_REPORT_STEPS_CIFP - steps);

  db.commit();

  return false;
}

bool XpDataCompiler::compileAirspaces()
{
  QStringList airspaceFiles = findAirspaceFiles(options);

  for(const QString& file : airspaceFiles)
  {
    if(options.isIncludedFilename(file))
    {
      // Only one progress report per file
      if(readDataFile(file, 1, airspaceWriter, READ_AIRSPACE | READ_SHORT_REPORT, 1))
        return true;
    }
  }
  db.commit();

  return false;
}

bool XpDataCompiler::compileLocalizers()
{
  QString path = buildPathNoCase({options.getBasepath(), "Custom Scenery", "Global Airports",
                                  "Earth nav data", "earth_nav.dat"});

  if(checkFile(path))
  {
    // Only one progress report per file
    bool aborted = readDataFile(path, 11, navWriter, READ_LOCALIZERS | READ_SHORT_REPORT, 1);
    if(!aborted)
      db.commit();
    return aborted;
  }

  return false;
}

bool XpDataCompiler::compileUserNav()
{
  QString path = buildPathNoCase({options.getBasepath(), "Custom Data", "user_nav.dat"});

  if(checkFile(path))
  {
    // One progress report per file
    bool aborted = readDataFile(path, 11, navWriter, READ_USER | READ_SHORT_REPORT, 1);
    if(!aborted)
      db.commit();
    return aborted;
  }

  return false;
}

bool XpDataCompiler::compileUserFix()
{
  QString path = buildPathNoCase({options.getBasepath(), "Custom Data", "user_fix.dat"});

  if(checkFile(path))
  {
    // One progress report per file
    bool aborted = readDataFile(path, 5, fixWriter, READ_USER | READ_SHORT_REPORT, 1);
    if(!aborted)
      db.commit();
    return aborted;
  }

  return false;
}

bool XpDataCompiler::compileMagDeclBgl()
{
  magDecReader->readFromWmm();
  magDecReader->writeToTable(db);
  db.commit();
  return false;
}

bool XpDataCompiler::readDataFile(const QString& filepath, int minColumns, XpWriter *writer,
                                  atools::fs::xp::ContextFlags flags, int numReportSteps)
{
  QFile file;
  QTextStream stream;
  stream.setCodec("UTF-8");
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
    // Open file and read header - throws exception on error
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

      if(flags & READ_SHORT_REPORT && numReportSteps > 0)
      {
        // One progress report per file - otherwise numReportSteps
        if(progress->reportOther(progressMsg))
          return true;
      }

      if(flags & READ_CIFP)
      {
        QString ident = QFileInfo(filepath).baseName().toUpper();
        if(!CIFP_MATCH.match(ident).hasMatch())
        {
          qWarning() << Q_FUNC_INFO << "CIFP file" << filepath << "has no valid name which should match airport ident";
          return aborted;
        }

        // Add additional information for procedure files
        context.cifpAirportIdent = QFileInfo(filepath).baseName().toUpper();
        context.cifpAirportId = airportIndex->getAirportId(context.cifpAirportIdent);
      }

      QString line;
      QStringList fields;

      QElapsedTimer timer;
      timer.start();
      qint64 elapsed = timer.elapsed();

      int rowsPerStep = 0;

      if(numReportSteps > 0)
        rowsPerStep = static_cast<int>(std::ceil(static_cast<float>(totalNumLines) /
                                                 static_cast<float>(numReportSteps)));
      int row = 0, steps = 0;

      // Read lines
      while(!stream.atEnd() && line != "99")
      {
        line = stream.readLine().trimmed();

        if(!(flags & READ_SHORT_REPORT) && numReportSteps > 0)
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

      if(!(flags & READ_SHORT_REPORT) && numReportSteps > 0)
        // Eat up any remaining progress steps
        progress->increaseCurrent(numReportSteps - steps);
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
  bool retval = false;

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

    if(!(flags & READ_CIFP) && !(flags & READ_AIRSPACE))
    {
      // Read file header =============================
      // Skip empty lines which can appear in some malformed add-on airport files
      // Byte order identifier ===========
      QString line;
      do
      {
        line = stream.readLine().simplified();
        lineNum++;
      } while(line.isEmpty() && !stream.atEnd() && line != "99");
      qInfo() << Q_FUNC_INFO << line;

      // Metadata and copyright ===========
      do
      {
        line = stream.readLine().simplified();
        lineNum++;
      } while(line.isEmpty() && !stream.atEnd() && line != "99");
      qInfo() << Q_FUNC_INFO << line;

      QStringList fields = line.simplified().split(" ");
      if(!fields.isEmpty())
        fileVersion = fields.constFirst().toInt();

      if(!fields.isEmpty() && fileVersion < minFileVersion)
      {
        qWarning() << "Version of" << filename << "is" << fields.constFirst() << "but expected a minimum of" << minFileVersion;
        throw atools::Exception(QString("Found file version %1. Minimum supported is %2.").arg(fields.constFirst()).arg(minFileVersion));
      }

      metadataWriter->writeFile(filename, QString(), curSceneryId, ++curFileId);
      progress->incNumFiles();
      retval = true;

      if(flags & UPDATE_CYCLE)
        updateAiracCycleFromHeader(line, filename, lineNum);

      qInfo() << Q_FUNC_INFO << "Counting lines for" << filename;
      qint64 pos = stream.pos();
      int lines = 0;
      while(!stream.atEnd())
      {
        line = stream.readLine();
        if(line == "99")
          break;
        lines++;
      }

      if(lines == 0)
      {
        qWarning() << Q_FUNC_INFO << "Empty file" << filepath;
        retval = false;
      }

      totalNumLines = lines;
      stream.seek(pos);
      qInfo() << Q_FUNC_INFO << "Num lines" << lines;
    }
    else
    {
      metadataWriter->writeFile(filename, QString(), curSceneryId, ++curFileId);
      progress->incNumFiles();
      retval = true;
    }
  }
  else
    throw atools::Exception("Cannot open file. Reason: " + filepath.errorString() + ".");

  return retval;
}

void XpDataCompiler::close()
{
  delete fixWriter;
  fixWriter = nullptr;

  delete moraWriter;
  moraWriter = nullptr;

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

  delete airportMsaWriter;
  airportMsaWriter = nullptr;

  delete holdingWriter;
  holdingWriter = nullptr;

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

QStringList XpDataCompiler::findCustomAptDatFiles(const atools::fs::NavDatabaseOptions& opts,
                                                  atools::fs::NavDatabaseErrors *navdatabaseErrors,
                                                  atools::fs::ProgressHandler *progressHandler)
{
  QStringList retval;

  if(!opts.isReadInactive())
    // Read only apt.dat from scenery_packs.ini - Global Airports are excluded and read separately
    retval = loadFilepathsFromSceneryPacks(opts, progressHandler, navdatabaseErrors);
  else
  {
    // Read all apt.dat files in the directory structure

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
  }
  return retval;
}

QStringList XpDataCompiler::findAirspaceFiles(const NavDatabaseOptions& opts)
{
  return findFiles(opts, "airspaces", {"*.txt"}, false /* makeUnique */);
}

QStringList XpDataCompiler::findCifpFiles(const NavDatabaseOptions& opts)
{
  return findFiles(opts, "CIFP", {"*.dat"}, true /* makeUnique */);
}

QStringList XpDataCompiler::findFiles(const NavDatabaseOptions& opts, const QString& subdir,
                                      const QStringList& pattern, bool makeUnique)
{
  QMap<QString, QFileInfo> entryMap;

  // Resources ==================================
  // Read all default entries
  QDir defaultDir(buildPathNoCase({opts.getBasepath(), "Resources", "default data", subdir}));
  QFileInfoList defaultEntries = defaultDir.entryInfoList(pattern, QDir::Files, QDir::NoSort);
  for(const QFileInfo& fileInfo : defaultEntries)
  {
    if(includeFile(opts, fileInfo))
      entryMap.insert(makeUnique ? fileInfo.fileName().toUpper() : fileInfo.filePath(), fileInfo);
  }

  // Custom Scenery ==================================
  // Read custom entries and overwrite default
  // Simply read all found filess in the scenery directories
  QDir customDir(buildPathNoCase({opts.getBasepath(), "Custom Data", subdir}));
  QFileInfoList customEntries = customDir.entryInfoList(pattern, QDir::Files, QDir::NoSort);
  for(const QFileInfo& fileInfo : customEntries)
  {
    if(includeFile(opts, fileInfo))
      entryMap.insert(makeUnique ? fileInfo.fileName().toUpper() : fileInfo.filePath(), fileInfo);
  }

  QStringList retval;
  for(const QFileInfo& fileInfo : entryMap)
    retval.append(fileInfo.filePath());

  // qDebug() << "== Files ==============================================================================";
  // qDebug() << Q_FUNC_INFO << retval;
  // qDebug() << "================================================================================";

  return retval;
}

int XpDataCompiler::calculateReportCount(ProgressHandler *progress, const NavDatabaseOptions& opts)
{
  FsPaths::SimulatorType sim = opts.getSimulatorType();

  int reportCount = 0;
  // Default or custom scenery files - required
  // earth_fix.dat earth_awy.dat earth_nav.dat earth_mora.dat
  reportCount += 4 * NUM_REPORT_STEPS_SMALL;

  bool aborted = false;

  if(sim == atools::fs::FsPaths::XPLANE_11)
  {
    if((aborted = progress->reportOtherMsg(tr("Counting files for Resources ..."))))
      return 0;

    // X-Plane 11/Resources/default scenery/default apt dat/Earth nav data/apt.dat ===================
    if(checkFile(buildPathNoCase({opts.getBasepath(), "Resources", "default scenery", "default apt dat", "Earth nav data", "apt.dat"})))
      reportCount += NUM_REPORT_STEPS;

    if((aborted = progress->reportOtherMsg(tr("Counting files for Custom Scenery/Global Airports ..."))))
      return 0;

    // X-Plane 11/Custom Scenery/Global Airports/Earth nav data/apt.dat ===================
    if(checkFile(buildPathNoCase({opts.getBasepath(), "Custom Scenery", "Global Airports", "Earth nav data", "apt.dat"})))
      reportCount += NUM_REPORT_STEPS;
  }

  if(sim == atools::fs::FsPaths::XPLANE_12)
  {
    // =========================================
    // As of X-Plane 12, the airport data is located in "$X-Plane/Global Scenery/Global Airports/Earth nav data/apt.dat"
    // and is the one and only source for both scenery and GPS data
    if((aborted = progress->reportOtherMsg(tr("Counting files for Global Scenery/Global Airports ..."))))
      return 0;

    // X-Plane 12/Global Scenery/Global Airports/Earth nav data/apt.dat
    if(checkFile(buildPathNoCase({opts.getBasepath(), "Global Scenery", "Global Airports", "Earth nav data", "apt.dat"})))
      reportCount += NUM_REPORT_STEPS;
  }

  // Default or custom CIFP/$ICAO.dat - required from either "Resources" or "Custom Data"
  reportCount += NUM_REPORT_STEPS_CIFP;

  if((aborted = progress->reportOtherMsg(tr("Counting files for Airspaces ..."))))
    return 0;

  reportCount += findAirspaceFiles(opts).count();

  if((aborted = progress->reportOtherMsg(tr("Counting files for Custom Scenery ..."))))
    return 0;

  // X-Plane 11/Custom Scenery/KSEA Demo Area/Earth nav data/apt.dat
  // X-Plane 11/Custom Scenery/LFPG Paris - Charles de Gaulle/Earth Nav data/apt.dat
  reportCount += findCustomAptDatFiles(opts, nullptr, nullptr).count();

  // earth_nav.dat localizers $X-Plane/Custom Scenery/Global Airports/Earth nav data/
  if(checkFile(buildPathNoCase({opts.getBasepath(), "Custom Scenery", "Global Airports", "Earth nav data", "earth_nav.dat"})))
    reportCount++;

  if((aborted = progress->reportOtherMsg(tr("Counting files for Custom Data ..."))))
    return 0;

  // user_nav.dat user_fix.dat $X-Plane/Custom Data/
  if(checkFile(buildPathNoCase({opts.getBasepath(), "Custom Data", "user_nav.dat"})))
    reportCount++;
  if(checkFile(buildPathNoCase({opts.getBasepath(), "Custom Data", "user_fix.dat"})))
    reportCount++;

  qDebug() << Q_FUNC_INFO << "=P=== X-Plane files" << reportCount;
  return reportCount;
}

QStringList XpDataCompiler::loadFilepathsFromSceneryPacks(const NavDatabaseOptions& opts,
                                                          ProgressHandler *progressHandler,
                                                          NavDatabaseErrors *navdatabaseErrors)
{
  qDebug() << Q_FUNC_INFO << "Reading scenery_packs.ini";
  QStringList entryMap;

  // Read X-Plane 11/Custom Scenery/scenery_packs.ini
  SceneryPacks sceneryPacks;
  sceneryPacks.read(opts.getBasepath());

  for(const SceneryPack& pack : sceneryPacks.getEntries())
  {
    if(pack.valid)
    {
      if(!pack.disabled)
      {
        QFileInfo fileInfo(pack.filepath);
        if(includeFile(opts, fileInfo))
          entryMap.append(fileInfo.filePath());
      }
      else
        qInfo() << "Disabled path" << pack.filepath;
    }
    else
    {
      if(progressHandler != nullptr)
        progressHandler->reportError();
      if(navdatabaseErrors != nullptr)
        navdatabaseErrors->sceneryErrors.first().fileErrors.append({pack.filepath, pack.errorText, pack.errorLine});

      qWarning() << Q_FUNC_INFO << "Error in file" << pack.filepath << "line" << pack.errorLine << ":" <<
        pack.errorText;
    }
  }
  return entryMap;
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
      qWarning() << Q_FUNC_INFO << "Error in file" << filepath << "line" << lineNum <<
        ": AIRAC cycle in file is empty.";
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

QString XpDataCompiler::buildBasePath(const NavDatabaseOptions& opts, const QString& filename)
{
  QString basePath;
  QString customPath(buildPathNoCase({opts.getBasepath(), "Custom Data"}));
  QString defaultPath(buildPathNoCase({opts.getBasepath(), "Resources", "default data"}));

  qDebug() << Q_FUNC_INFO << "customPath" << customPath;
  qDebug() << Q_FUNC_INFO << "defaultPath" << defaultPath;

  if(filename.isEmpty())
  {
    // No filename given - determine default path
    if(includeFile(opts, customPath) &&
       checkFile(buildPathNoCase({customPath, "earth_fix.dat"})) &&
       checkFile(buildPathNoCase({customPath, "earth_awy.dat"})) &&
       checkFile(buildPathNoCase({customPath, "earth_nav.dat"})))
      basePath = customPath;
    else if(checkFile(buildPathNoCase({defaultPath, "earth_fix.dat"})) &&
            checkFile(buildPathNoCase({defaultPath, "earth_awy.dat"})) &&
            checkFile(buildPathNoCase({defaultPath, "earth_nav.dat"})))
      basePath = defaultPath;
    else
      throw atools::Exception(tr("Cannot find valid files for X-Plane navdata in either\n\"%1\" or\n\"%2\"\n\n"
                                 "Make sure that earth_fix.dat, earth_awy.dat and earth_nav.dat "
                                 "can be found in one of these paths.").
                              arg(customPath).arg(defaultPath));
  }
  else
  {
    // Determine path for given file - return full filepath
    if(includeFile(opts, customPath) && checkFile(buildPathNoCase({customPath, filename})))
      basePath = customPath + QDir::separator() + filename;
    else if(checkFile(buildPathNoCase({defaultPath, filename})))
      basePath = defaultPath + QDir::separator() + filename;
  }
  qDebug() << Q_FUNC_INFO << "basePath" << basePath;

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
    if(!opts.isIncludedDirectoryGui(fileinfo.absoluteFilePath()))
      return false;
  }
  else if(fileinfo.isFile())
  {
    // Check if file is included from config file and GUI options
    if(!opts.isIncludedDirectoryGui(fileinfo.absolutePath()) ||
       !opts.isIncludedFilePathGui(fileinfo.absoluteFilePath()))
      return false;
  }

  return true;
}

} // namespace xp
} // namespace fs
} // namespace atools
