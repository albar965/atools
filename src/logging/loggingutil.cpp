/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include "logging/loggingutil.h"
#include "logging/logginghandler.h"

#include <QDebug>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QTextCodec>

namespace atools {
namespace logging {

void LoggingUtil::logSystemInformation()
{
  qInfo() << "Starting" << QCoreApplication::applicationName()
          << "version" << QCoreApplication::applicationVersion();
  qInfo() << "Organization" << QCoreApplication::organizationName()
          << "domain" << QCoreApplication::organizationDomain();
  qInfo() << "Application file path" << QCoreApplication::applicationFilePath();
  qInfo() << "Current working directory" << QDir(".").absolutePath();

  int i = 1;
  for(const QString& f : LoggingHandler::getLogFiles())
    qInfo() << "Log file" << i++ << f;

  qInfo() << "Default text codec" << QTextCodec::codecForLocale()->name();

  qInfo() << "ABI" << QSysInfo::buildAbi();
  qInfo() << "build CPU arch" << QSysInfo::buildCpuArchitecture()
          << "current" <<
  QSysInfo::currentCpuArchitecture();

  qInfo() << "kernel" << QSysInfo::kernelType()
          << "version" << QSysInfo::kernelVersion();

  qInfo() << "product name" << QSysInfo::prettyProductName()
          << "type" << QSysInfo::productType()
          << "version" << QSysInfo::productVersion();

  qInfo() << "Qt version" << QT_VERSION_STR;

  if(QSysInfo::windowsVersion() != QSysInfo::WV_None)
    qInfo() << "Windows version" << QSysInfo::windowsVersion();

  if(QSysInfo::macVersion() != QSysInfo::MV_None)
    qInfo() << "Mac version" << QSysInfo::macVersion();
}

void LoggingUtil::logStandardPaths()
{
  qInfo() << "DesktopLocation" << QStandardPaths::standardLocations(QStandardPaths::DesktopLocation);
  qInfo() << "DocumentsLocation" << QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
  qInfo() << "FontsLocation" << QStandardPaths::standardLocations(QStandardPaths::FontsLocation);
  qInfo() << "ApplicationsLocation" << QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
  qInfo() << "MusicLocation" << QStandardPaths::standardLocations(QStandardPaths::MusicLocation);
  qInfo() << "MoviesLocation" << QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
  qInfo() << "PicturesLocation" << QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
  qInfo() << "TempLocation" << QStandardPaths::standardLocations(QStandardPaths::TempLocation);
  qInfo() << "HomeLocation" << QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
  qInfo() << "DataLocation" << QStandardPaths::standardLocations(QStandardPaths::DataLocation);
  qInfo() << "CacheLocation" << QStandardPaths::standardLocations(QStandardPaths::CacheLocation);
  qInfo() << "GenericDataLocation" << QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
  qInfo() << "RuntimeLocation" << QStandardPaths::standardLocations(QStandardPaths::RuntimeLocation);
  qInfo() << "ConfigLocation" << QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
  qInfo() << "DownloadLocation" << QStandardPaths::standardLocations(QStandardPaths::DownloadLocation);
  qInfo() << "GenericCacheLocation" << QStandardPaths::standardLocations(QStandardPaths::GenericCacheLocation);
  qInfo() << "GenericConfigLocation" << QStandardPaths::standardLocations(
    QStandardPaths::GenericConfigLocation);
  qInfo() << "AppDataLocation" << QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
  qInfo() << "AppConfigLocation" << QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);
}

} // namespace logging
} // namespace atools
