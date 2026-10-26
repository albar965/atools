/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "fs/weather/xpweatherreader.h"

#include "atools.h"
#include "fs/weather/metar.h"
#include "util/filesystemwatcher.h"
#include "fs/weather/metarindex.h"
#include "fs/util/fsutil.h"

#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QDebug>

namespace atools {
namespace fs {
namespace weather {

// Allow one minute tolerance before trying to reload weather files
const qint64 WEATHER_TIME_EPSILON = 60LL;

using atools::util::FileSystemWatcher;

XpWeatherReader::XpWeatherReader(QObject *parent, bool verboseLogging)
  : QObject(parent), verbose(verboseLogging)
{
  metarIndex = new atools::fs::weather::MetarIndex(atools::fs::weather::XPLANE, verboseLogging);
}

XpWeatherReader::~XpWeatherReader()
{
  clear();
  delete metarIndex;
}

const atools::fs::weather::Metar& XpWeatherReader::getXplaneMetar(const QString& station, const atools::geo::Pos& pos)
{
  prepareLoad();
  return metarIndex->getMetar(station, pos);
}

void XpWeatherReader::simulatorTimeUpdated()
{
  // Call load if not initialized yet or on timeout of WEATHER_TIME_EPSILON
  if(needsLoading() ||
     atools::almostNotEqual(weatherDatetimeLastLoadTry.toSecsSinceEpoch(), weatherDatetimeCurrent.toSecsSinceEpoch(), WEATHER_TIME_EPSILON))
    load();
}

void XpWeatherReader::connectedToSimulator()
{

}

void XpWeatherReader::disconnectedFromSimulator()
{
  weatherDatetimeCurrent = weatherDatetimeLastLoadTry = QDateTime::currentDateTimeUtc();
  load();
}

void XpWeatherReader::setWeatherPath(const QString& path, XpWeatherType type)
{
  clear();
  weatherPath = path;
  weatherType = type;
}

bool XpWeatherReader::needsLoading() const
{
  return fileWatcher == nullptr && !weatherPath.isEmpty();
}

void XpWeatherReader::prepareLoad()
{
  if(!weatherDatetimeCurrent.isValid())
    weatherDatetimeCurrent = QDateTime::currentDateTimeUtc();

  if(!weatherDatetimeLastLoadTry.isValid())
    weatherDatetimeLastLoadTry = QDateTime::currentDateTimeUtc();

  if(needsLoading())
    load();
}

void XpWeatherReader::load()
{
  // Remember time when *trying* to load
  weatherDatetimeLastLoadTry = weatherDatetimeCurrent;
  const QList<MetarFile> metarFiles = collectWeatherFiles();
  if(metarFiles != currentMetarFiles)
  {
    currentMetarFiles = metarFiles;
    metarIndex->clear();

    if(verbose)
      qDebug() << Q_FUNC_INFO << weatherPath;

    createFsWatcher();
  }
}

void XpWeatherReader::clear()
{
  if(verbose)
    qDebug() << Q_FUNC_INFO;
  deleteFsWatcher();
  metarIndex->clear();
  weatherPath.clear();
  currentMetarFiles.clear();
}

void XpWeatherReader::setFetchAirportCoords(atools::fs::util::AirportCoordFuncType function, void *object)
{
  metarIndex->setFetchAirportCoords(function, object);
}

bool XpWeatherReader::read(const QStringList& filenames)
{
  // Read and merge all changed filenames into the METAR index
  for(const QString& filename : filenames)
  {
    qInfo() << Q_FUNC_INFO << "Loading METAR file" << filename;

    QFile file(filename);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      if(verbose)
        qDebug() << Q_FUNC_INFO << filename;
      QTextStream stream(&file);

      // Read and merge into current METAR entries
      metarIndex->read(stream, filename, true /* merge */);
      file.close();
    }
    else
      qWarning() << "cannot open" << file.fileName() << "reason" << file.errorString();
  }
  return !metarIndex->isEmpty();
}

void XpWeatherReader::dirUpdated(const QString& dir)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << dir;

  // Update list of files and reload weather if this has changed
  const QList<MetarFile> metarFiles = collectWeatherFiles();
  if(metarFiles != currentMetarFiles)
  {
    currentMetarFiles = metarFiles;
    createFsWatcher();
  }
}

void XpWeatherReader::filesUpdated(const QStringList& filenames)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << filenames;

  if(read(filenames))
    emit weatherUpdated();
  else if(verbose)
    qDebug() << Q_FUNC_INFO << "File not changed";
}

QList<XpWeatherReader::MetarFile> XpWeatherReader::collectWeatherFiles() const
{
  QList<MetarFile> files;

  if(weatherType == atools::fs::weather::WEATHER_XP11)
    // METAR.rwx
    files.append(MetarFile(weatherPath));
  else if(weatherType == atools::fs::weather::WEATHER_XP12)
  {
    // Collect all files sorted from old to new ========================
    QMap<qint64, QFileInfo> metarFiles;
    qint64 weatherDatetimeSecs = weatherDatetimeCurrent.toSecsSinceEpoch();

    // METAR-2022-9-6-19.00-ZULU.txt, METAR-2022-9-6-20.00-ZULU.txt
    // METAR-2022-9-6-19.00.txt, METAR-2022-9-6-20.00.txt
    const QDir weatherDir(weatherPath, "METAR-*.txt", QDir::Unsorted, QDir::Files | QDir::NoDotAndDotDot);
    const QFileInfoList entries = weatherDir.entryInfoList();
    for(const QFileInfo& entry : entries)
      metarFiles.insert(atools::fs::util::xpMetarFilenameToDateTime(entry.fileName()).toSecsSinceEpoch(), entry);

    if(!metarFiles.isEmpty())
    {
      const QList<qint64> keys = metarFiles.keys();

      // Count backwards from future to past
      for(int i = keys.size() - 1; i >= 0; i--)
      {
        if(keys.at(i) <= weatherDatetimeSecs)
        {
          // Time is equal or earlier than current - add and maintain list from late to early for merging
          files.prepend(MetarFile(metarFiles.value(keys.at(i))));
          if(files.size() >= 3)
            break;
        }
      }

      // Nothing found since time is too early - add oldest file
      if(files.isEmpty() && weatherDatetimeSecs < keys.constFirst())
        files.append(MetarFile(metarFiles.value(keys.constFirst())));
    }
  }

  return files;
}

void XpWeatherReader::deleteFsWatcher()
{
  if(fileWatcher != nullptr)
  {
    atools::util::FileSystemWatcher::disconnect(fileWatcher, &FileSystemWatcher::filesUpdated, this, &XpWeatherReader::filesUpdated);
    atools::util::FileSystemWatcher::disconnect(fileWatcher, &FileSystemWatcher::dirUpdated, this, &XpWeatherReader::dirUpdated);
    fileWatcher->deleteLater();
    fileWatcher = nullptr;
  }
}

void XpWeatherReader::createFsWatcher()
{
  if(fileWatcher == nullptr)
  {
    // Watch file for changes and directory too to catch file deletions
    fileWatcher = new FileSystemWatcher(this, verbose);

    // Set to smaller value to deal with ASX weather files
    fileWatcher->setMinFileSize(1000);
    atools::util::FileSystemWatcher::connect(fileWatcher, &FileSystemWatcher::filesUpdated, this, &XpWeatherReader::filesUpdated);
    atools::util::FileSystemWatcher::connect(fileWatcher, &FileSystemWatcher::dirUpdated, this, &XpWeatherReader::dirUpdated);
  }

  // Load initially
  QStringList files;
  for(const MetarFile& file : std::as_const(currentMetarFiles))
    files.append(file.getFilename());

  filesUpdated(files);

  if(currentMetarFiles.isEmpty() && weatherType == WEATHER_XP12)
    // Watch at least folder if file list is empty
    fileWatcher->setFilenamesAndStart({weatherPath});
  else if(!currentMetarFiles.isEmpty())
    // Start watching for changes
    fileWatcher->setFilenamesAndStart({currentMetarFiles.constFirst().getFilename()});
}

void XpWeatherReader::debugDumpContainerSizes() const
{
  qDebug() << Q_FUNC_INFO << "currentMetarFiles.size()" << currentMetarFiles.size();
  qDebug() << Q_FUNC_INFO << "metarIndex.size()" << metarIndex->numStationMetars();
}

} // namespace weather
} // namespace fs
} // namespace atools
