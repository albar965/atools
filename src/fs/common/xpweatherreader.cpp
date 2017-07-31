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

#include "fs/common/xpweatherreader.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>
#include <QFileSystemWatcher>

namespace atools {
namespace fs {
namespace common {

XpWeatherReader::XpWeatherReader(QObject *parent)
  : QObject(parent)
{

}

XpWeatherReader::~XpWeatherReader()
{
  clear();
}

void atools::fs::common::XpWeatherReader::readWeatherFile(const QString& file)
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
  metars.clear();
  weatherFile.clear();
}

QString XpWeatherReader::getMetar(const QString& ident)
{
  return metars.value(ident);
}

// 2017/07/30 18:45
// KHYI 301845Z 13007KT 070V130 10SM SCT075 38/17 A2996
//
// 2017/07/30 18:55
// KPRO 301855Z AUTO 11003KT 10SM CLR 26/14 A3022 RMK AO2 T02570135
//
// 2017/07/30 18:47
// KADS 301847Z 06005G14KT 13SM SKC 32/19 A3007
void XpWeatherReader::read()
{
  // Recognize METAR airport
  static const QRegularExpression IDENT_REGEXP("^[A-Z0-9]{2,5}$");

  // Recognize date part
  static const QRegularExpression DATE_REGEXP("^[\\d]{4}/[\\d]{2}/[\\d]{2}");

  QFile file(weatherFile);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QTextStream stream(&file);

    int lineNum = 1;
    QString line;
    while(!stream.atEnd())
    {
      line = stream.readLine().trimmed();

      if(line.isEmpty())
        continue;

      if(line.size() >= 4)
      {
        if(DATE_REGEXP.match(line).hasMatch())
          // Ignore date rows
          continue;

        QString ident = line.section(' ', 0, 0);
        if(IDENT_REGEXP.match(ident).hasMatch())
          // Starts with an airport ident
          metars.insert(ident, line);
        else
          qWarning() << "Metar does not match in file" << file.fileName() << "line num" << lineNum << "line" << line;
      }
      lineNum++;
    }
    file.close();

  }
  else
    qWarning() << "cannot open" << file.fileName() << "reason" << file.errorString();
}

/* Called on directory or file change */
void XpWeatherReader::pathChanged(const QString& path)
{
  Q_UNUSED(path);
  QFileInfo fileinfo(weatherFile);
  if(fileinfo.exists() && fileinfo.isFile())
  {
    // File exists
    if(fileinfo.lastModified() > weatherFileTimestamp)
    {
      // Timestamp of file has changed
      qDebug() << Q_FUNC_INFO << "reading" << weatherFile;
      read();
      weatherFileTimestamp = fileinfo.lastModified();
      emit weatherUpdated();
    }
  }
  else
  {
    // File does not exist
    if(!metars.isEmpty())
    {
      qDebug() << Q_FUNC_INFO << "removed" << weatherFile;
      metars.clear();
      weatherFileTimestamp = QDateTime();
      emit weatherUpdated();
    }
  }
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
}

void XpWeatherReader::createFsWatcher()
{
  if(fsWatcher == nullptr)
  {
    // Watch file for changes and directory too to catch file deletions
    fsWatcher = new QFileSystemWatcher(this);
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
}

} // namespace common
} // namespace fs
} // namespace atools
