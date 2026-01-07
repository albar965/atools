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

#include "fs/fspaths.h"

#include "atools.h"
#include "settings/settings.h"

#include <QDir>
#include <QDataStream>
#include <QSettings>
#include <QProcess>
#include <QStringBuilder>

#if defined(Q_OS_WIN32)
#include <windows.h>
#include <QStandardPaths>
#endif

namespace atools {
namespace fs {

using atools::SEP;

static QHash<atools::fs::FsPaths::SimulatorType, QString> basePathMap;
static QHash<atools::fs::FsPaths::SimulatorType, QString> filesPathMap;
static QHash<atools::fs::FsPaths::SimulatorType, QString> sceneryFilepathMap;

/* All supported simulators. Order in this vector defines order of detection. */
static const QList<atools::fs::FsPaths::SimulatorType> ALL_SIMULATOR_TYPES({
      FsPaths::FSX, FsPaths::FSX_SE, FsPaths::P3D_V3, FsPaths::P3D_V4, FsPaths::P3D_V5, FsPaths::P3D_V6,
      FsPaths::XPLANE_11, FsPaths::XPLANE_12, FsPaths::MSFS, FsPaths::MSFS_2024
    });

/* All supported MS simulators using SimConnect on Windows */
static const QSet<atools::fs::FsPaths::SimulatorType> ALL_SIMULATOR_TYPES_MS({
      FsPaths::FSX, FsPaths::FSX_SE, FsPaths::P3D_V3, FsPaths::P3D_V4, FsPaths::P3D_V5, FsPaths::P3D_V6, FsPaths::MSFS, FsPaths::MSFS_2024
    });

/* All supported MSFS simulators using SimConnect on Windows */
static const QSet<atools::fs::FsPaths::SimulatorType> ALL_SIMULATOR_TYPES_MSFS({
      FsPaths::MSFS, FsPaths::MSFS_2024
    });

/* All supported FSX simulators using SimConnect on Windows */
static const QSet<atools::fs::FsPaths::SimulatorType> ALL_SIMULATOR_TYPES_FSX({
      FsPaths::FSX, FsPaths::FSX_SE
    });

/* All supported P3D simulators using SimConnect on Windows */
static const QSet<atools::fs::FsPaths::SimulatorType> ALL_SIMULATOR_TYPES_P3D({
      FsPaths::P3D_V3, FsPaths::P3D_V4, FsPaths::P3D_V5, FsPaths::P3D_V6
    });

/* All supported X-Plane simulators using Xpconnect */
static const QSet<atools::fs::FsPaths::SimulatorType> ALL_SIMULATOR_TYPES_XP({
      FsPaths::XPLANE_11, FsPaths::XPLANE_12
    });

static const QHash<atools::fs::FsPaths::SimulatorType, QString> ALL_SIMULATOR_TYPE_NAMES(
    {
      {FsPaths::FSX, QStringLiteral("FSX")},
      {FsPaths::FSX_SE, QStringLiteral("FSXSE")},
      {FsPaths::P3D_V3, QStringLiteral("P3DV3")},
      {FsPaths::P3D_V4, QStringLiteral("P3DV4")},
      {FsPaths::P3D_V5, QStringLiteral("P3DV5")},
      {FsPaths::P3D_V6, QStringLiteral("P3DV6")},
      {FsPaths::XPLANE_11, QStringLiteral("XP11")},
      {FsPaths::XPLANE_12, QStringLiteral("XP12")},
      {FsPaths::MSFS, QStringLiteral("MSFS")},
      {FsPaths::MSFS_2024, QStringLiteral("MSFS24")},
      {FsPaths::NAVIGRAPH, QStringLiteral("NAVIGRAPH")}
    });

static const QHash<atools::fs::FsPaths::SimulatorType, QString> ALL_SIMULATOR_DISPLAY_NAMES(
    {
      {FsPaths::FSX, QStringLiteral("Microsoft Flight Simulator X")},
      {FsPaths::FSX_SE, QStringLiteral("Microsoft Flight Simulator - Steam Edition")},
      {FsPaths::P3D_V3, QStringLiteral("Prepar3D v3")},
      {FsPaths::P3D_V4, QStringLiteral("Prepar3D v4")},
      {FsPaths::P3D_V5, QStringLiteral("Prepar3D v5")},
      {FsPaths::P3D_V6, QStringLiteral("Prepar3D v6")},
      {FsPaths::XPLANE_11, QStringLiteral("X-Plane 11")},
      {FsPaths::XPLANE_12, QStringLiteral("X-Plane 12")},
      {FsPaths::MSFS, QStringLiteral("Microsoft Flight Simulator 2020")},
      {FsPaths::MSFS_2024, QStringLiteral("Microsoft Flight Simulator 2024")},
      {FsPaths::NAVIGRAPH, QStringLiteral("Navigraph")}
    }
  );

static const QHash<atools::fs::FsPaths::SimulatorType, QString> ALL_SIMULATOR_SHORT_DISPLAY_NAMES(
    {
      {FsPaths::FSX, QStringLiteral("FSX")},
      {FsPaths::FSX_SE, QStringLiteral("FSX SE")},
      {FsPaths::P3D_V3, QStringLiteral("P3D v3")},
      {FsPaths::P3D_V4, QStringLiteral("P3D v4")},
      {FsPaths::P3D_V5, QStringLiteral("P3D v5")},
      {FsPaths::P3D_V6, QStringLiteral("P3D v6")},
      {FsPaths::XPLANE_11, QStringLiteral("X-Plane 11")},
      {FsPaths::XPLANE_12, QStringLiteral("X-Plane 12")},
      {FsPaths::MSFS, QStringLiteral("MSFS 2020")},
      {FsPaths::MSFS_2024, QStringLiteral("MSFS 2024")},
      {FsPaths::NAVIGRAPH, QStringLiteral("Navigraph")}
    }
  );

static QString msfsOfficialPath, msfsCommunityPath, msfsSimPath, msfs24SimPath, msfs24CommunityPath, msfs24LocalStatePath;

/* Platform: FSX, FSX XPack, FSX Gold */
const QLatin1String FSX_REGISTRY_PATH("HKEY_CURRENT_USER\\Software\\Microsoft");
const QStringList FSX_REGISTRY_KEY = {"Microsoft Games", "Flight Simulator", "10.0", "AppPath"};

/* Platform: FSX Steam Edition */
const QLatin1String FSX_SE_REGISTRY_PATH("HKEY_CURRENT_USER\\Software\\Microsoft");
const QStringList FSX_SE_REGISTRY_KEY = {"Microsoft Games", "Flight Simulator - Steam Edition", "10.0", "AppPath"};

/* Platform: Prepar3d Version 3 */
const QLatin1String P3D_V3_REGISTRY_PATH("HKEY_CURRENT_USER\\Software");
const QStringList P3D_V3_REGISTRY_KEY = {"Lockheed Martin", "Prepar3D v3", "AppPath"};

/* Platform: Prepar3d Version 4 */
const QLatin1String P3D_V4_REGISTRY_PATH("HKEY_CURRENT_USER\\Software");
const QStringList P3D_V4_REGISTRY_KEY = {"Lockheed Martin", "Prepar3D v4", "AppPath"};

/* Platform: Prepar3d Version 5 */
const QLatin1String P3D_V5_REGISTRY_PATH("HKEY_CURRENT_USER\\Software");
const QStringList P3D_V5_REGISTRY_KEY = {"Lockheed Martin", "Prepar3D v5", "AppPath"};

/* Platform: Prepar3d Version 6 */
const QLatin1String P3D_V6_REGISTRY_PATH("HKEY_CURRENT_USER\\Software");
const QStringList P3D_V6_REGISTRY_KEY = {"Lockheed Martin", "Prepar3D v6", "AppPath"};

/* Settings keys */
const QLatin1String SETTINGS_FSX_PATH("FsPaths/FsxPath");
const QLatin1String SETTINGS_FSX_SE_PATH("FsPaths/FsxSePath");
const QLatin1String SETTINGS_P3D_V3_PATH("FsPaths/P3dV3Path");
const QLatin1String SETTINGS_P3D_V4_PATH("FsPaths/P3dV4Path");
const QLatin1String SETTINGS_P3D_V5_PATH("FsPaths/P3dV5Path");
const QLatin1String SETTINGS_P3D_V6_PATH("FsPaths/P3dV6Path");
const QLatin1String SETTINGS_XPLANE_11_PATH("FsPaths/XPlane11Path");
const QLatin1String SETTINGS_XPLANE_12_PATH("FsPaths/XPlane12Path");
const QLatin1String SETTINGS_MSFS_PATH("FsPaths/MsfsPath");
const QLatin1String SETTINGS_MSFS24_PATH("FsPaths/Msfs24Path");

/* Paths for non Windows systems - used for development and debugging purposes */
const QLatin1String FSX_NO_WINDOWS_PATH("Microsoft Flight Simulator X");
const QLatin1String FSX_SE_NO_WINDOWS_PATH("Flight Simulator - Steam Edition");
const QLatin1String P3D_V3_NO_WINDOWS_PATH("Prepar3D v3");
const QLatin1String P3D_V4_NO_WINDOWS_PATH("Prepar3D v4");
const QLatin1String P3D_V5_NO_WINDOWS_PATH("Prepar3D v5");
const QLatin1String P3D_V6_NO_WINDOWS_PATH("Prepar3D v6");
const QLatin1String MSFS_NO_WINDOWS_PATH("MSFS2020");
const QLatin1String MSFS24_NO_WINDOWS_PATH("MSFS2024");

static FsPaths::MsfsInstallType msfsInstallType = FsPaths::MSFS_INSTALL_NONE;
static FsPaths::MsfsInstallType msfs24InstallType = FsPaths::MSFS_INSTALL_NONE;

static QProcessEnvironment environment;

using atools::settings::Settings;

void FsPaths::logAllPaths()
{
  qDebug() << Q_FUNC_INFO << "====================================================";

  // C:\ProgramData
  qInfo() << "PROGRAMDATA" << environment.value("PROGRAMDATA");

  // C:\Users\USER\AppData\Roaming
  qInfo() << "APPDATA" << environment.value("APPDATA");

  // C:\Users\USER\AppData\Local
  qInfo() << "LOCALAPPDATA" << environment.value("LOCALAPPDATA"); // MSFS

  // C:\ProgramData
  qInfo() << "ALLUSERSPROFILE" << environment.value("ALLUSERSPROFILE");

  for(atools::fs::FsPaths::SimulatorType type : ALL_SIMULATOR_TYPES)
  {
    QString basePath = basePathMap.value(type);
    QString filesPath = filesPathMap.value(type);
    QString sceneryFilepath = sceneryFilepathMap.value(type);

    qInfo() << "==============================================";
    qInfo().nospace().noquote() << ALL_SIMULATOR_TYPE_NAMES[type] << " - " << ALL_SIMULATOR_DISPLAY_NAMES[type];
    if(basePath.isEmpty())
      qInfo() << "  Base is empty";
    else
      qInfo() << "  Base" << basePath << "exists" << QFileInfo::exists(basePath);

    if(filesPath.isEmpty())
      qInfo() << "  Files path is empty";
    else
      qInfo() << "  Files" << filesPath << "exists" << QFileInfo::exists(filesPath);

    if(sceneryFilepath.isEmpty())
      qInfo() << "  Scenery.cfg path is empty";
    else
      qInfo() << "  Scenery.cfg" << sceneryFilepath << "exists" << QFileInfo::exists(sceneryFilepath);

    if(type == MSFS)
    {
      qInfo() << "  msfsCommunityPath" << msfsCommunityPath;
      qInfo() << "  msfsOfficialPath" << msfsOfficialPath;
    }

    if(type == MSFS_2024)
      qInfo() << "  msfs24CommunityPath" << msfs24CommunityPath;
  }
}

void FsPaths::loadAllPaths()
{
  qInfo() << Q_FUNC_INFO << "Start ======================================";

  basePathMap.clear();
  filesPathMap.clear();
  sceneryFilepathMap.clear();

  for(atools::fs::FsPaths::SimulatorType type : ALL_SIMULATOR_TYPES)
  {
    qInfo() << Q_FUNC_INFO << "============";
    basePathMap.insert(type, atools::nativeCleanPath(initBasePath(type)));
    filesPathMap.insert(type, atools::nativeCleanPath(initFilesPath(type)));
    sceneryFilepathMap.insert(type, atools::nativeCleanPath(initSceneryLibraryPath(type)));
  }
  qInfo() << Q_FUNC_INFO << "Done ======================================";
}

void FsPaths::intitialize()
{
  environment = QProcessEnvironment::systemEnvironment();
}

QString FsPaths::getBasePath(FsPaths::SimulatorType type)
{
  return basePathMap.value(type);
}

QString FsPaths::getFilesPath(FsPaths::SimulatorType type)
{
  return filesPathMap.value(type);
}

QString FsPaths::getSceneryLibraryPath(FsPaths::SimulatorType type)
{
  return sceneryFilepathMap.value(type);
}

QString FsPaths::getMsfsOfficialPath()
{
  return msfsOfficialPath;
}

FsPaths::MsfsInstallType FsPaths::getMsfsInstallType()
{
  return msfsInstallType;
}

FsPaths::MsfsInstallType FsPaths::getMsfs24InstallType()
{
  return msfs24InstallType;
}

QString FsPaths::getMsfsOfficialPath(const QString& basePath)
{
  // Also check subfolders and layout file to avoid confusion if folders from other installations remain
  // Look first for Steam since some might have remains from MS subscription around
#ifdef DEBUG_SILENCE_COMPILER_WARNINGS
  bool warn = false;
#else
  bool warn = true;
#endif

  QString msgSteam = checkFileMsg(basePath % SEP % QStringLiteral("Official") % SEP % QStringLiteral("Steam") % SEP %
                                  QStringLiteral("fs-base") % SEP % QStringLiteral("layout.json"), 80, warn);
  QString msgOneStore = checkFileMsg(basePath % SEP % QStringLiteral("Official") % SEP % QStringLiteral("OneStore") % SEP %
                                     QStringLiteral("fs-base") % SEP % QStringLiteral("layout.json"), 80, warn);

  if(msgSteam.isEmpty())
    return basePath % SEP % QStringLiteral("Official") % SEP % QStringLiteral("Steam");
  else if(msgOneStore.isEmpty())
    return basePath % SEP % QStringLiteral("Official") % SEP % QStringLiteral("OneStore");
  else
  {
    qWarning() << Q_FUNC_INFO << msgSteam << msgOneStore;
    return QString();
  }
}

QString FsPaths::getMsfsCommunityPath()
{
  return msfsCommunityPath;
}

QString FsPaths::getMsfs24CommunityPath()
{
  return msfs24CommunityPath;
}

QString FsPaths::getMsfsCommunityPath(const QString& basePath)
{
#ifdef DEBUG_SILENCE_COMPILER_WARNINGS
  bool warn = false;
#else
  bool warn = true;
#endif

  QString msg = checkDirMsg(basePath % SEP % QStringLiteral("Community"), 80, warn);

  if(msg.isEmpty())
    return basePath % SEP % QStringLiteral("Community");
  else
  {
    qWarning() << Q_FUNC_INFO << msg;
    return QString();
  }
}

QString FsPaths::initBasePath(SimulatorType type)
{
  QString fsPath;
  if(type == NAVIGRAPH || type == NONE)
    return QString();

  if(type == XPLANE_11) // ===============================================================================================
  {
    // =====================================================
    // The location of this file varies by operating system:
    // OS X - the file will be in the user’s preferences folder, e.g. ~/Library/Preferences/.
    // Windows -  the file will be in the user’s local app data folder, e.g. C:\Users\nnnn\AppData\Local\.
    // Use CSIDL_LOCAL_APPDATA to find the location.
    // Linux - the file will be in the user’s home folder in a .x-plane/ sub folder, e.g. ~/.x-plane/.

#if defined(Q_OS_WIN32)
    // "C:\Users\USERS\AppData\Local\x-plane_install_11.txt"
    return xplaneBasePath(environment.value(QStringLiteral("LOCALAPPDATA")) % SEP % QStringLiteral("x-plane_install_11.txt"));

#elif defined(Q_OS_MACOS)
    // "/Users/USER/Library/Preferences/x-plane_install_11.txt"
    return xplaneBasePath(QDir::homePath() % SEP % QStringLiteral("Library") % SEP % QStringLiteral("Preferences") % SEP %
                          QStringLiteral("x-plane_install_11.txt"));

#elif defined(Q_OS_LINUX)
    // "/home/USER/.x-plane/x-plane_install_11.txt"
    return xplaneBasePath(QDir::homePath() % SEP % QStringLiteral(".x-plane") % SEP % QStringLiteral("x-plane_install_11.txt"));

#endif
  }
  else if(type == XPLANE_12) // ===============================================================================================
  {
#if defined(Q_OS_WIN32)
    // "C:\Users\USERS\AppData\Local\x-plane_install_12.txt"
    return xplaneBasePath(environment.value(QStringLiteral("LOCALAPPDATA")) % SEP % QStringLiteral("x-plane_install_12.txt"));

#elif defined(Q_OS_MACOS)
    // "/Users/USER/Library/Preferences/x-plane_install_12.txt"
    return xplaneBasePath(QDir::homePath() % SEP % QStringLiteral("Library") % SEP % QStringLiteral("Preferences") % SEP %
                          QStringLiteral("x-plane_install_12.txt"));

#elif defined(Q_OS_LINUX)
    // "/home/USER/.x-plane/x-plane_install_12.txt"
    return xplaneBasePath(QDir::homePath() % SEP % QStringLiteral(".x-plane") % SEP % QStringLiteral("x-plane_install_12.txt"));

#endif
  }
  else if(type == MSFS) // ===============================================================================================
  {
    // Read UserCfg.opt to find the packages installation path
#if defined(Q_OS_WIN32)
    QString temp;

    // MS online installation ====================
    // C:\Users\USER\AppData\Local\Packages\Microsoft.FlightSimulator_8wekyb3d8bbwe\LocalCache\UserCfg.opt
    temp = msfsBasePath(environment.value(QStringLiteral("LOCALAPPDATA")) % SEP % QStringLiteral("Packages") % SEP %
                        QStringLiteral("Microsoft.FlightSimulator_8wekyb3d8bbwe") % SEP % QStringLiteral("LocalCache") % SEP %
                        QStringLiteral("UserCfg.opt"), type);
    if(checkDir(Q_FUNC_INFO, temp))
    {
      fsPath = temp;
      qInfo() << Q_FUNC_INFO << "Found MSFS path" << fsPath;

      // C:\Users\USER\AppData\Local\Packages\Microsoft.FlightSimulator_8wekyb3d8bbwe
      msfsSimPath = environment.value(QStringLiteral("LOCALAPPDATA")) % SEP % QStringLiteral("Packages") % SEP %
                    QStringLiteral("Microsoft.FlightSimulator_8wekyb3d8bbwe");
      qInfo() << Q_FUNC_INFO << "Found MSFS Online simulator path" << msfsSimPath;

      msfsInstallType = MSFS_INSTALL_ONLINE;
    }

    // Steam installation ====================
    // C:\Users\USER\AppData\Roaming\Microsoft Flight Simulator\UserCfg.opt
    temp = msfsBasePath(environment.value(QStringLiteral("APPDATA")) % SEP % QStringLiteral("Microsoft Flight Simulator") % SEP %
                        QStringLiteral("UserCfg.opt"), type);
    if(checkDir(Q_FUNC_INFO, temp))
    {
      fsPath = temp;
      qInfo() << Q_FUNC_INFO << "Found MSFS path" << fsPath;

      // C:\Users\USER\AppData\Roaming\Microsoft Flight Simulator
      msfsSimPath = environment.value(QStringLiteral("APPDATA")) % SEP % QStringLiteral("Microsoft Flight Simulator");
      qInfo() << Q_FUNC_INFO << "Found MSFS Steam simulator path" << msfsSimPath;

      msfsInstallType = MSFS_INSTALL_STEAM;
    }

    // MS Boxed installation ====================
    // C:\Users\USER\AppData\Local\MSFSPackages\UserCfg.opt
    temp = msfsBasePath(environment.value(QStringLiteral("LOCALAPPDATA")) % SEP % QStringLiteral("MSFSPackages") % SEP %
                        QStringLiteral("UserCfg.opt"), type);
    if(checkDir(Q_FUNC_INFO, temp))
    {
      fsPath = temp;
      qInfo() << Q_FUNC_INFO << "Found MSFS path" << fsPath;

      // C:\Users\USER\AppData\Local\MSFSPackages\UserCfg.opt
      msfsSimPath = environment.value(QStringLiteral("LOCALAPPDATA")) % SEP % QStringLiteral("MSFSPackages");
      qInfo() << Q_FUNC_INFO << "Found MSFS Boxed simulator path" << msfsSimPath;

      msfsInstallType = MSFS_INSTALL_BOXED;
    }

#elif defined(DEBUG_FS_PATHS)

    QString temp;
    QString nonWinPath = nonWindowsPathFull(type);

    /// home/USERNAME/Simulators/MSFS2020\Packages\Microsoft.FlightSimulator_8wekyb3d8bbwe\LocalCache\UserCfg.opt
    temp = msfsBasePath(nonWinPath % SEP % QStringLiteral("Packages") % SEP %
                        QStringLiteral("Microsoft.FlightSimulator_8wekyb3d8bbwe") % SEP % QStringLiteral("LocalCache") % SEP %
                        QStringLiteral("UserCfg.opt"), type);
    if(checkDir(Q_FUNC_INFO, temp))
    {
      fsPath = temp;
      qInfo() << Q_FUNC_INFO << "Found MSFS path" << fsPath;

      // C:\Users\USER\AppData\Local\Packages\Microsoft.FlightSimulator_8wekyb3d8bbwe
      msfsSimPath = nonWinPath % SEP % QStringLiteral("Packages") % SEP % QStringLiteral("Microsoft.FlightSimulator_8wekyb3d8bbwe");
      qInfo() << Q_FUNC_INFO << "Found MSFS simulator path" << msfsSimPath;

      msfsInstallType = MSFS_INSTALL_ONLINE;
    }

#endif

    if(!fsPath.isEmpty())
    {
      // fsPath = ...\Microsoft.FlightSimulator_8wekyb3d8bbwe\LocalCache\Packages
      msfsCommunityPath = fsPath % SEP % QStringLiteral("Community");
      qInfo() << Q_FUNC_INFO << "Found MSFS community path" << msfsCommunityPath;

      // Find one of the official path variations for MS and Steam
      msfsOfficialPath = getMsfsOfficialPath(fsPath);
      qInfo() << Q_FUNC_INFO << "Found MSFS official path" << msfsOfficialPath;
    }
  }
  else if(type == MSFS_2024) // ===============================================================================================
  {
    // Read UserCfg.opt to find the packages installation path
#if defined(Q_OS_WIN32)
    QString temp;

    // MS online installation ====================
    // C:\Users\USER\AppData\Local\Packages\Microsoft.Limitless_8wekyb3d8bbwe\LocalCache\UserCfg.opt
    temp = msfsBasePath(environment.value(QStringLiteral("LOCALAPPDATA")) % SEP % QStringLiteral("Packages") % SEP %
                        QStringLiteral("Microsoft.Limitless_8wekyb3d8bbwe") % SEP % QStringLiteral("LocalCache") % SEP %
                        QStringLiteral("UserCfg.opt"), type);
    if(checkDir(Q_FUNC_INFO, temp))
    {
      fsPath = temp;
      qInfo() << Q_FUNC_INFO << "Found MSFS 2024 path" << fsPath;

      // C:\Users\USER\AppData\Local\Packages\Microsoft.Limitless_8wekyb3d8bbwe
      msfs24SimPath = environment.value(QStringLiteral("LOCALAPPDATA")) % SEP % QStringLiteral("Packages") % SEP %
                      QStringLiteral("Microsoft.Limitless_8wekyb3d8bbwe");
      qInfo() << Q_FUNC_INFO << "Found MSFS 2024 Online simulator path" << msfs24SimPath;

      msfs24InstallType = MSFS_INSTALL_ONLINE;
    }

    // Steam installation ====================
    // C:\Users\USER\AppData\Roaming\Microsoft Flight Simulator 2024\UserCfg.opt
    temp = msfsBasePath(environment.value(QStringLiteral("APPDATA")) % SEP % QStringLiteral("Microsoft Flight Simulator 2024") % SEP %
                        QStringLiteral("UserCfg.opt"), type);
    if(checkDir(Q_FUNC_INFO, temp))
    {
      fsPath = temp;
      qInfo() << Q_FUNC_INFO << "Found MSFS 2024 path" << fsPath;

      // C:\Users\USER\AppData\Roaming\Microsoft Flight Simulator 2024
      msfs24SimPath = environment.value(QStringLiteral("APPDATA")) % SEP % QStringLiteral("Microsoft Flight Simulator 2024");
      qInfo() << Q_FUNC_INFO << "Found MSFS 2024 Steam simulator path" << msfs24SimPath;

      msfs24InstallType = MSFS_INSTALL_STEAM;
    }

#elif defined(DEBUG_FS_PATHS)

    QString temp;
    QString nonWinPath = nonWindowsPathFull(type);

    /// home/USERNAME/Simulators/MSFS2024\Packages\Microsoft.Limitless_8wekyb3d8bbwe.Limitless_8wekyb3d8bbwe\LocalCache\UserCfg.opt
    temp = msfsBasePath(nonWinPath % SEP % QStringLiteral("Packages") % SEP %
                        QStringLiteral("Microsoft.Limitless_8wekyb3d8bbwe") % SEP % QStringLiteral("LocalCache") % SEP %
                        QStringLiteral("UserCfg.opt"), type);
    if(checkDir(Q_FUNC_INFO, temp))
    {
      fsPath = temp;
      qInfo() << Q_FUNC_INFO << "Found MSFS 2024 path" << fsPath;

      // C:\Users\USER\AppData\Local\Packages\Microsoft.Limitless_8wekyb3d8bbwe
      msfs24SimPath = nonWinPath % SEP % QStringLiteral("Packages") % SEP % QStringLiteral("Microsoft.Limitless_8wekyb3d8bbwe");
      qInfo() << Q_FUNC_INFO << "Found MSFS 2024 simulator path" << msfs24SimPath;

      msfs24InstallType = MSFS_INSTALL_ONLINE;
    }
#endif
    if(!fsPath.isEmpty())
    {
      // C:\Users\USERNAME\AppData\Local\Packages\Microsoft.Limitless_8wekyb3d8bbwe\LocalState\StreamedPackages
      // StreamedPackages were removed later
      msfs24LocalStatePath = msfs24SimPath % SEP % QStringLiteral("LocalState");
      checkDir(Q_FUNC_INFO, msfs24LocalStatePath);

      msfs24CommunityPath = fsPath % SEP % QStringLiteral("Community");
      qInfo() << Q_FUNC_INFO << "Found MSFS 2024 community path" << msfs24CommunityPath;

      qInfo() << Q_FUNC_INFO << "Found MSFS 2024 LocalState Path" << msfs24LocalStatePath;
    }

    fsPath = msfs24LocalStatePath;
  }
  else
  {
    // Windows simulators =====================================================
#if defined(Q_OS_WIN32)
    // Try to get the FSX path from the Windows registry
    QSettings settings(registryPath(type), QSettings::NativeFormat);

    QStringList keys(registryKey(type));
    bool found = true;

    // Last entry is the value
    // Avoid using value on the whole tree since it creates empty entries
    for(int i = 0; i < keys.size() - 1; i++)
    {
      if(settings.childGroups().contains(keys.at(i), Qt::CaseInsensitive))
        settings.beginGroup(keys.at(i));
      else
      {
        found = false;
        break;
      }
    }

    if(found && !keys.isEmpty() && settings.contains(keys.constLast()))
    {
      fsPath = settings.value(keys.constLast()).toString();

      if(fsPath.endsWith('\\'))
        fsPath.chop(1);
    }
#elif defined(DEBUG_FS_PATHS)
    // No Windows here - get the path for debugging purposes =====================================================
    // from the configuration file
    fsPath = nonWindowsPathFull(type);
#endif

    if(checkDir(Q_FUNC_INFO, fsPath))
      qInfo() << Q_FUNC_INFO << "Found" << typeToShortName(type) << "base path" << fsPath;
    else
    {
      fsPath.clear();
      qInfo() << Q_FUNC_INFO << typeToShortName(type) << "base path not found";
    }
  }
  return fsPath;
}

QString FsPaths::nonWindowsPathFull(atools::fs::FsPaths::SimulatorType type)
{
  QString fsPath;
  // from the configuration file
  Settings& s = Settings::instance();
  QString key = settingsKey(type);

  if(!key.isEmpty())
  {
    fsPath = s.valueStr(key);
    if(fsPath.isEmpty())
    {
      // If it is not present in the settings file use one of the predefined paths
      // Useful with symlinks for debugging
      QString home = QDir::homePath() % SEP % QStringLiteral("Simulators");
      QString nonWinPath = nonWindowsPath(type);

      if(!nonWinPath.isEmpty())
      {
        QFileInfo fi(home % SEP % nonWinPath);
        if(checkDir(Q_FUNC_INFO, fi))
          fsPath = fi.absoluteFilePath();
      }
    }
  }
  return fsPath;
}

bool FsPaths::hasSimulator(FsPaths::SimulatorType type)
{
  return !getBasePath(type).isEmpty();
}

bool FsPaths::hasAnyMsSimulator()
{
  for(atools::fs::FsPaths::SimulatorType type : ALL_SIMULATOR_TYPES_MS)
  {
    if(!basePathMap.value(type).isEmpty())
      return true;
  }
  return false;
}

bool FsPaths::hasAnyXplaneSimulator()
{
  return hasXplane11Simulator() || hasXplane12Simulator();
}

bool FsPaths::hasXplane11Simulator()
{
  return !basePathMap.value(XPLANE_11).isEmpty();
}

bool FsPaths::hasXplane12Simulator()
{
  return !basePathMap.value(XPLANE_12).isEmpty();
}

bool FsPaths::isAnyMs(SimulatorType type)
{
  return ALL_SIMULATOR_TYPES_MS.contains(type);
}

bool FsPaths::isAnyP3d(SimulatorType type)
{
  return ALL_SIMULATOR_TYPES_P3D.contains(type);
}

bool FsPaths::isAnyFsx(SimulatorType type)
{
  return ALL_SIMULATOR_TYPES_FSX.contains(type);
}

bool FsPaths::isAnyMsfs(SimulatorType type)
{
  return ALL_SIMULATOR_TYPES_MSFS.contains(type);
}

bool FsPaths::isAnyXplane(SimulatorType type)
{
  return ALL_SIMULATOR_TYPES_XP.contains(type);
}

QString FsPaths::initFilesPath(SimulatorType type)
{
  QString fsFilesDir;

  switch(type)
  {
    case atools::fs::FsPaths::XPLANE_11:
    case atools::fs::FsPaths::XPLANE_12:
      fsFilesDir = atools::buildPathNoCase({getBasePath(type), QStringLiteral("Output"), QStringLiteral("FMS Plans")});
      break;

    case atools::fs::FsPaths::MSFS:
      // %localappdata%
      // C:\Users\USER\AppData\Local\Packages\Microsoft.FlightSimulator_8wekyb3d8bbwe\LocalState
      fsFilesDir = msfsSimPath % SEP % QStringLiteral("LocalState");
      if(!checkDir(Q_FUNC_INFO, fsFilesDir))
        // Steam uses top level as path
        // C:\Users\USER\AppData\Roaming\Microsoft Flight Simulator
        fsFilesDir = msfsSimPath;
      break;

    case atools::fs::FsPaths::MSFS_2024:
      // %localappdata%
      // C:\Users\USER\AppData\Local\Packages\Microsoft.Limitless_8wekyb3d8bbwe\LocalState
      fsFilesDir = msfs24SimPath % SEP % QStringLiteral("LocalState");
      if(!checkDir(Q_FUNC_INFO, fsFilesDir))
        // Steam uses top level as path
        // C:\Users\USER\AppData\Roaming\Microsoft Flight Simulator 2024
        fsFilesDir = msfs24SimPath;
      break;

    case atools::fs::FsPaths::FSX:
    case atools::fs::FsPaths::FSX_SE:
    case atools::fs::FsPaths::P3D_V3:
    case atools::fs::FsPaths::P3D_V4:
    case atools::fs::FsPaths::P3D_V5:
    case atools::fs::FsPaths::P3D_V6:
#if defined(Q_OS_WIN32)
      {
        QString languageDll(getBasePath(type) % SEP % QStringLiteral("language.dll"));
        qDebug() << "Language DLL" << languageDll;

        // Copy to wchar and append null
        wchar_t languageDllWChar[1024];
        languageDll.toWCharArray(languageDllWChar);
        languageDllWChar[languageDll.size()] = L'\0';

        // Load the FS language DLL
        HINSTANCE hInstLanguageDll = LoadLibrary(languageDllWChar);
        if(hInstLanguageDll)
        {
          qDebug() << "Got handle from LoadLibrary";

          // Get the language dependent files name from the language.dll resources
          // (parts of code from Peter Dowson in fsdeveloper forum)
          wchar_t filesPathWChar[MAX_PATH];
          LoadStringW(hInstLanguageDll, 36864, filesPathWChar, MAX_PATH);
          FreeLibrary(hInstLanguageDll);

          // Check all Document folders for path - there should be only one
          for(QString document : QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation))
          {
            QFileInfo fsFilesDirInfo(document % SEP % QString::fromWCharArray(filesPathWChar));
            if(checkDir(Q_FUNC_INFO, fsFilesDirInfo))
            {
              fsFilesDir = fsFilesDirInfo.absoluteFilePath();
              qDebug() << "Found" << fsFilesDir;
              break;
            }
            else
              qDebug() << "Does not exist" << fsFilesDir;
          }
        }
        else
          qDebug() << "No handle from LoadLibrary";
      }
#elif defined(DEBUG_FS_PATHS)
      fsFilesDir = atools::documentsDir() % SEP % typeToShortName(type);

#endif
      break;

    case atools::fs::FsPaths::NAVIGRAPH:
    case atools::fs::FsPaths::ALL_SIMULATORS:
    case atools::fs::FsPaths::NONE:
      break;

  }

  // Use fallback on non Windows systems or if not found
  if(fsFilesDir.isEmpty())
  {
    fsFilesDir = atools::documentsDir();
    qWarning() << Q_FUNC_INFO << "Files path for" << typeToShortName(type) << "not found. Fallback to" << fsFilesDir;
  }
  else
    qInfo() << Q_FUNC_INFO << "Found" << typeToShortName(type) << "files path" << fsFilesDir;

  return fsFilesDir;
}

QString FsPaths::initSceneryLibraryPath(SimulatorType type)
{
  QString sceneryPath;
#if defined(Q_OS_WIN32)
  // Win 7+ C:\ProgramData
  QString programData(environment.value(QStringLiteral("PROGRAMDATA")));

  // Win 7+ C:\Users\{username}\AppData\Roaming
  QString appData(environment.value(QStringLiteral("APPDATA")));

  QString allUsersProfile(environment.value(QStringLiteral("ALLUSERSPROFILE")));

  if(programData.isEmpty())
    programData = allUsersProfile % SEP % QDir(appData).dirName();
#endif

  switch(type)
  {
    case FSX:
      // FSX C:\Users\user account name\AppData\Roaming\Microsoft\FSX\scenery.cfg
      // or C:\ProgramData\Microsoft\FSX\Scenery.cfg
#if defined(Q_OS_WIN32)
      sceneryPath = programData % SEP % QStringLiteral("Microsoft\\FSX\\Scenery.CFG");

#elif defined(DEBUG_FS_PATHS)
      sceneryPath = getBasePath(type) % SEP % QStringLiteral("scenery.cfg");
#endif
      break;

    case FSX_SE:
      // FSX SE C:\ProgramData\Microsoft\FSX-SE\Scenery.cfg
#if defined(Q_OS_WIN32)
      sceneryPath = programData % SEP % QStringLiteral("Microsoft\\FSX-SE\\Scenery.CFG");

#elif defined(DEBUG_FS_PATHS)
      sceneryPath = getBasePath(type) % SEP % QStringLiteral("scenery.cfg");
#endif
      break;

    case P3D_V3:
      // P3D v3 C:\ProgramData\Lockheed Martin\Prepar3D v3
#if defined(Q_OS_WIN32)
      sceneryPath = programData % SEP % QStringLiteral("Lockheed Martin\\Prepar3D v3\\Scenery.CFG");

#elif defined(DEBUG_FS_PATHS)
      sceneryPath = getBasePath(type) % SEP % QStringLiteral("scenery.cfg");
#endif
      break;

    case P3D_V4:
      // P3D v4 C:\ProgramData\Lockheed Martin\Prepar3D v4
#if defined(Q_OS_WIN32)
      sceneryPath = programData % SEP % QStringLiteral("Lockheed Martin\\Prepar3D v4\\Scenery.CFG");

#elif defined(DEBUG_FS_PATHS)
      sceneryPath = getBasePath(type) % SEP % QStringLiteral("scenery.cfg");
#endif
      break;

    case P3D_V5:
      // P3D v5 C:\ProgramData\Lockheed Martin\Prepar3D v5
#if defined(Q_OS_WIN32)
      sceneryPath = programData % SEP % QStringLiteral("Lockheed Martin\\Prepar3D v5\\Scenery.CFG");

#elif defined(DEBUG_FS_PATHS)
      sceneryPath = getBasePath(type) % SEP % QStringLiteral("scenery.cfg");
#endif
      break;

    case P3D_V6:
      // P3D v6 C:\ProgramData\Lockheed Martin\Prepar3D v6
      // %PROGRAMDATA%\Lockheed Martin\Prepar3D v6
#if defined(Q_OS_WIN32)
      sceneryPath = programData % SEP % QStringLiteral("Lockheed Martin\\Prepar3D v6\\Scenery.CFG");

#elif defined(DEBUG_FS_PATHS)
      sceneryPath = getBasePath(type) % SEP % QStringLiteral("scenery.cfg");
#endif
      break;

    // Disable compiler warnings - simulators that dont have a file reference
    case MSFS:
    case MSFS_2024:
    case XPLANE_11:
    case XPLANE_12:
    case NAVIGRAPH:
    case NONE:
    case ALL_SIMULATORS:
      break;
  }

  if(sceneryPath.isEmpty())
  {
    if(!atools::contains(type, {MSFS, MSFS_2024, XPLANE_11, XPLANE_12}))
      qWarning() << Q_FUNC_INFO << "Scenery path for" << typeToShortName(type) << "not found.";
  }
  else
    qInfo() << Q_FUNC_INFO << "Found" << typeToShortName(type) << "scenery path" << sceneryPath;

  return sceneryPath;
}

QString FsPaths::typeToShortName(SimulatorType type)
{
  return ALL_SIMULATOR_TYPE_NAMES.value(type);
}

QString FsPaths::typeToDisplayName(SimulatorType type)
{
  return ALL_SIMULATOR_DISPLAY_NAMES.value(type);
}

QString FsPaths::typeToShortDisplayName(SimulatorType type)
{
  return ALL_SIMULATOR_SHORT_DISPLAY_NAMES.value(type);
}

FsPaths::SimulatorType FsPaths::stringToType(const QString& typeStr)
{
  QString type = typeStr.toUpper();
  if(type == QStringLiteral("FSX"))
    return FSX;
  else if(type == QStringLiteral("FSXSE"))
    return FSX_SE;
  else if(type == QStringLiteral("P3DV3"))
    return P3D_V3;
  else if(type == QStringLiteral("P3DV4"))
    return P3D_V4;
  else if(type == QStringLiteral("P3DV5"))
    return P3D_V5;
  else if(type == QStringLiteral("P3DV6"))
    return P3D_V6;
  else if(type == QStringLiteral("XP11"))
    return XPLANE_11;
  else if(type == QStringLiteral("XP12"))
    return XPLANE_12;
  else if(type == QStringLiteral("MSFS"))
    return MSFS;
  else if(type == QStringLiteral("MSFS24"))
    return MSFS_2024;
  else if(type == QStringLiteral("NAVIGRAPH") || type == QStringLiteral("DFD"))
    return NAVIGRAPH;
  else
    return NONE;
}

const QList<FsPaths::SimulatorType>& FsPaths::getAllSimulatorTypes()
{
  return ALL_SIMULATOR_TYPES;
}

QString FsPaths::settingsKey(SimulatorType type)
{
  switch(type)
  {
    case FSX:
      return SETTINGS_FSX_PATH;

    case FSX_SE:
      return SETTINGS_FSX_SE_PATH;

    case P3D_V3:
      return SETTINGS_P3D_V3_PATH;

    case P3D_V4:
      return SETTINGS_P3D_V4_PATH;

    case P3D_V5:
      return SETTINGS_P3D_V5_PATH;

    case P3D_V6:
      return SETTINGS_P3D_V6_PATH;

    case XPLANE_11:
      return SETTINGS_XPLANE_11_PATH;

    case XPLANE_12:
      return SETTINGS_XPLANE_12_PATH;

    case MSFS:
      return SETTINGS_MSFS_PATH;

    case MSFS_2024:
      return SETTINGS_MSFS24_PATH;

    case NAVIGRAPH:
    case NONE:
    case ALL_SIMULATORS:
      break;
  }
  return QString();
}

QString FsPaths::registryPath(SimulatorType type)
{
  switch(type)
  {
    case FSX:
      return FSX_REGISTRY_PATH;

    case FSX_SE:
      return FSX_SE_REGISTRY_PATH;

    case P3D_V3:
      return P3D_V3_REGISTRY_PATH;

    case P3D_V4:
      return P3D_V4_REGISTRY_PATH;

    case P3D_V5:
      return P3D_V5_REGISTRY_PATH;

    case P3D_V6:
      return P3D_V6_REGISTRY_PATH;

    case MSFS:
    case MSFS_2024:
    case XPLANE_11:
    case XPLANE_12:
    case NAVIGRAPH:
    case NONE:
    case ALL_SIMULATORS:
      break;
  }
  return QString();
}

QStringList FsPaths::registryKey(SimulatorType type)
{
  switch(type)
  {
    case FSX:
      return FSX_REGISTRY_KEY;

    case FSX_SE:
      return FSX_SE_REGISTRY_KEY;

    case P3D_V3:
      return P3D_V3_REGISTRY_KEY;

    case P3D_V4:
      return P3D_V4_REGISTRY_KEY;

    case P3D_V5:
      return P3D_V5_REGISTRY_KEY;

    case P3D_V6:
      return P3D_V6_REGISTRY_KEY;

    case MSFS:
    case MSFS_2024:
    case XPLANE_11:
    case XPLANE_12:
    case NAVIGRAPH:
    case ALL_SIMULATORS:
    case NONE:
      break;
  }
  return QStringList();
}

QString FsPaths::nonWindowsPath(SimulatorType type)
{
  switch(type)
  {
    case FSX:
      return FSX_NO_WINDOWS_PATH;

    case FSX_SE:
      return FSX_SE_NO_WINDOWS_PATH;

    case P3D_V3:
      return P3D_V3_NO_WINDOWS_PATH;

    case P3D_V4:
      return P3D_V4_NO_WINDOWS_PATH;

    case P3D_V5:
      return P3D_V5_NO_WINDOWS_PATH;

    case P3D_V6:
      return P3D_V6_NO_WINDOWS_PATH;

    case MSFS:
      return MSFS_NO_WINDOWS_PATH;

    case MSFS_2024:
      return MSFS24_NO_WINDOWS_PATH;

    case XPLANE_11:
    case XPLANE_12:
    case NAVIGRAPH:
    case NONE:
    case ALL_SIMULATORS:
      break;
  }
  return QString();
}

QString FsPaths::msfsBasePath(const QString& userCfgOptFile, SimulatorType type)
{
  qInfo() << Q_FUNC_INFO << "Checking MSFS path from" << userCfgOptFile;

  QString dir;
  QFile fileCfgOpt(userCfgOptFile);
  if(fileCfgOpt.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QTextStream stream(&fileCfgOpt);
    while(!stream.atEnd())
    {
      QString line = stream.readLine().trimmed();

      if(line.startsWith(QStringLiteral("InstalledPackagesPath")))
      {
        QString name = line.section(' ', 1).trimmed();
        if(name.startsWith('"'))
          name.remove(0, 1);

        if(name.endsWith('"'))
          name.chop(1);

        QFileInfo fi(name);
        if(fi.exists() && fi.isDir())
        {
          dir = fi.absoluteFilePath();
          break;
        }
        else
          qWarning() << Q_FUNC_INFO << fi.absoluteFilePath() << "does not exist or is not a directory";
      }
    }

    fileCfgOpt.close();

    qInfo() << Q_FUNC_INFO << "Found MSFS base path" << dir;

    // Official/Steam or Official/OneStore is required =================
    if(type == MSFS)
    {
      if(!dir.isEmpty())
      {
        if(getMsfsOfficialPath(dir).isEmpty())
        {
          dir.clear();
          qWarning() << Q_FUNC_INFO << "MSFS official path not found";
        }
      }

      // Community is not required - loading process will show a warning about the missing folder =================
      if(!dir.isEmpty())
      {
        if(getMsfsCommunityPath(dir).isEmpty())
          qWarning() << Q_FUNC_INFO << "MSFS community path not found";
      }
    }
  }
  else
    qWarning() << Q_FUNC_INFO << "Cannot open" << userCfgOptFile << "error" << fileCfgOpt.errorString();

  return dir;
}

QString FsPaths::xplaneBasePath(const QString& installationFile)
{
  qInfo() << Q_FUNC_INFO << "Checking XP path from" << installationFile;

  QString dir;
  QFile file(installationFile);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QTextStream stream(&file);

    while(!stream.atEnd())
    {
      QFileInfo fi(stream.readLine().trimmed());
      if(checkDir(Q_FUNC_INFO, fi))
      {
        dir = fi.absoluteFilePath();
        break;
      }
      else
        qWarning() << Q_FUNC_INFO << fi.absoluteFilePath() << "does not exist or is not a directory";
    }

    file.close();

    qInfo() << Q_FUNC_INFO << "Found XP base path" << dir;
  }
  else
    qWarning() << Q_FUNC_INFO << "Cannot open" << installationFile << "error" << file.errorString();

  return dir;
}

} // namespace fs
} // namespace atools

QDataStream& operator<<(QDataStream& out, const atools::fs::FsPaths::SimulatorType& obj)
{
  out << static_cast<int>(obj);
  return out;
}

QDataStream& operator>>(QDataStream& in, atools::fs::FsPaths::SimulatorType& obj)
{
  int val;
  in >> val;
  obj = static_cast<atools::fs::FsPaths::SimulatorType>(val);
  return in;
}
