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

#include "fs/weather/metar.h"
#include "util/filesystemwatcher.h"
#include "fs/weather/metarindex.h"
#include "fs/util/fsutil.h"

#include <algorithm>

#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QDebug>

namespace atools {
namespace fs {
namespace weather {

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

void XpWeatherReader::setWeatherPath(const QString& path, XpWeatherType type)
{
  clear();
  weatherPath = path;
  weatherType = type;
}

bool XpWeatherReader::needsLoading()
{
  return fileWatcher == nullptr && !weatherPath.isEmpty();
}

void XpWeatherReader::load()
{
  if(fileWatcher == nullptr && !weatherPath.isEmpty())
  {
    metarIndex->clear();
    currentMetarFiles = collectWeatherFiles();

    if(verbose)
      qDebug() << Q_FUNC_INFO << weatherPath << currentMetarFiles;

    createFsWatcher();
  }
}

const atools::fs::weather::Metar& XpWeatherReader::getXplaneMetar(const QString& station, const atools::geo::Pos& pos)
{
  if(needsLoading())
    load();

  return metarIndex->getMetar(station, pos);
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

void XpWeatherReader::setFetchAirportCoords(const std::function<geo::Pos(const QString&)>& value)
{
  metarIndex->setFetchAirportCoords(value);
}

bool XpWeatherReader::read(const QStringList& filenames)
{
  // Read and merge all changed filenames into the METAR index
  for(const QString& filename : filenames)
  {
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
  QStringList metarFiles = collectWeatherFiles();
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

QStringList XpWeatherReader::collectWeatherFiles()
{
  QFileInfoList metarFiles;

  if(weatherType == atools::fs::weather::WEATHER_XP11)
    // METAR.rwx
    metarFiles.append(QFileInfo(weatherPath));
  else if(weatherType == atools::fs::weather::WEATHER_XP12)
  {
    // METAR-2022-9-6-19.00-ZULU.txt, METAR-2022-9-6-20.00-ZULU.txt
    // METAR-2022-9-6-19.00.txt, METAR-2022-9-6-20.00.txt
    QDir weatherDir(weatherPath, "METAR-*.txt", QDir::Name, QDir::Files | QDir::NoDotAndDotDot);
    const QFileInfoList entries = weatherDir.entryInfoList();
    for(const QFileInfo& entry : entries)
      metarFiles.append(entry);

    // Sort by timestamp - put latest at begin of list
    if(metarFiles.size() > 1)
      std::sort(metarFiles.begin(), metarFiles.end(), [](const QFileInfo& file1, const QFileInfo& file2)->bool {
              return atools::fs::util::xpMetarFilenameToDate(file1.fileName()) > atools::fs::util::xpMetarFilenameToDate(file2.fileName());
            });
  }

  if(verbose)
    qDebug() << Q_FUNC_INFO << metarFiles;

  // Return only latest three files
  QStringList files;
  for(int i = 0; i < std::min(3, static_cast < int > (metarFiles.size())); i++)
    files.append(metarFiles.at(i).absoluteFilePath());

  if(verbose)
    qDebug() << Q_FUNC_INFO << files;

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
  filesUpdated(currentMetarFiles);

  if(currentMetarFiles.isEmpty() && weatherType == WEATHER_XP12)
    // Watch at least folder if file list is empty
    fileWatcher->setFilenamesAndStart({weatherPath});
  else
    // Start watching for changes
    fileWatcher->setFilenamesAndStart(currentMetarFiles);
}

void XpWeatherReader::debugDumpContainerSizes() const
{
  qDebug() << Q_FUNC_INFO << "currentMetarFiles.size()" << currentMetarFiles.size();
  qDebug() << Q_FUNC_INFO << "metarIndex.size()" << metarIndex->numStationMetars();
}

} // namespace weather
} // namespace fs
} // namespace atools
