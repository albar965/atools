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

#include "fs/weather/xpweatherreader.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>
#include <QFileSystemWatcher>

namespace atools {
namespace fs {
namespace weather {

XpWeatherReader::XpWeatherReader(QObject *parent, bool verboseLogging)
  : QObject(parent), index(5000), verbose(verboseLogging)
{
  timer.setObjectName("XpWeatherReader.timer");
}

XpWeatherReader::~XpWeatherReader()
{
  clear();
}

void XpWeatherReader::readWeatherFile(const QString& file)
{
  clear();
  weatherFile = file;
  createFsWatcher();

  QFileInfo fileinfo(file);
  if(fileinfo.exists() && fileinfo.isFile())
    read();
  // else wait for file created
}

void XpWeatherReader::clear()
{
  deleteFsWatcher();
  index.clear();
  weatherFile.clear();
}

atools::fs::weather::MetarResult XpWeatherReader::getXplaneMetar(const QString& station, const atools::geo::Pos& pos)
{
  atools::fs::weather::MetarResult result;
  result.requestIdent = station;
  result.requestPos = pos;

  MetarData data;
  QString foundKey = index.getTypeOrNearest(data, station, pos);
  if(!foundKey.isEmpty())
  {
    // Found a METAR
    if(foundKey == station)
      // Found exact match
      result.metarForStation = data.metar;
    else
      // Found a station nearby
      result.metarForNearest = data.metar;
  }

  result.timestamp = QDateTime::currentDateTime();
  return result;
}

QString XpWeatherReader::getMetar(const QString& ident)
{
  return index.value(ident).metar;
}

// 2017/07/30 18:45
// KHYI 301845Z 13007KT 070V130 10SM SCT075 38/17 A2996
//
// 2017/07/30 18:55
// KPRO 301855Z AUTO 11003KT 10SM CLR 26/14 A3022 RMK AO2 T02570135
//
// 2017/07/30 18:47
// KADS 301847Z 06005G14KT 13SM SKC 32/19 A3007
bool XpWeatherReader::read()
{
  // Recognize METAR airport
  static const QRegularExpression IDENT_REGEXP("^[A-Z0-9]{2,5}$");

  // Recognize date part
  static const QRegularExpression DATE_REGEXP("^[\\d]{4}/[\\d]{2}/[\\d]{2}");

  QFile file(weatherFile);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    index.clear();

    QTextStream stream(&file);

    int lineNum = 1;
    QString line;
    QDateTime lastTimestamp;

    while(!stream.atEnd())
    {
      line = stream.readLine().trimmed();

      if(line.isEmpty())
        continue;

      if(line.size() >= 4)
      {
        if(DATE_REGEXP.match(line).hasMatch())
        {
          // 2017/10/29 11:45
          lastTimestamp = QDateTime::fromString(line, "yyyy/MM/dd hh:mm");
          continue;
        }

        QString ident = line.section(' ', 0, 0);
        if(IDENT_REGEXP.match(ident).hasMatch())
        {
          if(index.contains(ident))
          {
            MetarData md = index.value(ident);
            if(md.timestamp > lastTimestamp)
              // Ignore. Already loaded is newer.
              continue;
          }

          // Starts with an airport ident - add if position is valid
          atools::geo::Pos pos = fetchAirportCoords(ident);
          if(pos.isValid())
            index.insert(ident, {ident, line, lastTimestamp}, pos);
        }
        else
          qWarning() << "Metar does not match in file" << file.fileName() << "line num" << lineNum << "line" << line;
      }
      lineNum++;
    }
    file.close();
  }
  else
  {
    qWarning() << "cannot open" << file.fileName() << "reason" << file.errorString();
    return false;
  }

  qDebug() << Q_FUNC_INFO << "Loaded" << index.size() << "metars";
  return true;
}

/* Called on directory or file change and QTimer timer */
void XpWeatherReader::pathChanged()
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << sender()->objectName();

  QFileInfo fileinfo(weatherFile);
  if(fileinfo.exists() && fileinfo.isFile() && fileinfo.size() > MIN_FILE_SIZE)
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << "File exists" << fileinfo.exists()
               << "size" << fileinfo.size() << "last modified" << fileinfo.lastModified();

    // File exists - first call or older than two minutes or file differs
    if(!weatherFileTimestamp.isValid() ||
       fileinfo.lastModified() > weatherFileTimestamp ||
       lastFileSize != fileinfo.size())
    {
      // Timestamp of file has changed
      qDebug() << Q_FUNC_INFO << "reading" << weatherFile;
      if(read())
      {
        weatherFileTimestamp = fileinfo.lastModified();
        lastFileSize = fileinfo.size();
        emit weatherUpdated();
      }
    }
    else if(verbose)
      qDebug() << Q_FUNC_INFO << "File not changed";
  }
  else
    // File was deleted - keep current weather information
    qDebug() << Q_FUNC_INFO << "File does not exist. Index empty:" << index.isEmpty();
}

void XpWeatherReader::deleteFsWatcher()
{
  if(fsWatcher != nullptr)
  {
    fsWatcher->disconnect(fsWatcher, &QFileSystemWatcher::fileChanged, this, &XpWeatherReader::pathChanged);
    fsWatcher->disconnect(fsWatcher, &QFileSystemWatcher::directoryChanged, this, &XpWeatherReader::pathChanged);
    fsWatcher->deleteLater();
    fsWatcher = nullptr;
  }
  timer.stop();
  timer.disconnect(&timer, &QTimer::timeout, this, &XpWeatherReader::pathChanged);
}

void XpWeatherReader::createFsWatcher()
{
  if(fsWatcher == nullptr)
  {
    // Watch file for changes and directory too to catch file deletions
    fsWatcher = new QFileSystemWatcher(this);
    fsWatcher->setObjectName("XpWeatherReader.fsWatcher");

    fsWatcher->connect(fsWatcher, &QFileSystemWatcher::fileChanged, this, &XpWeatherReader::pathChanged);
    fsWatcher->connect(fsWatcher, &QFileSystemWatcher::directoryChanged, this, &XpWeatherReader::pathChanged);
  }

  // Watch file to get changes
  if(!fsWatcher->addPath(weatherFile))
    qWarning() << "cannot watch" << weatherFile;

  // Watch directory to get added or removed file changes
  QFileInfo fileinfo(weatherFile);
  if(!fsWatcher->addPath(fileinfo.path()))
    qWarning() << "cannot watch" << fileinfo.path();

  // Check every ten seconds since the watcher is unreliable
  timer.setInterval(10000);
  timer.connect(&timer, &QTimer::timeout, this, &XpWeatherReader::pathChanged);
  timer.start();
}

} // namespace weather
} // namespace fs
} // namespace atools
