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

#include "fspaths.h"

#include "settings/settings.h"
#include "atools.h"

#include <QDebug>
#include <QHash>
#include <QDir>
#include <QStandardPaths>
#include <QDataStream>
#include <QSettings>
#include <QtGlobal>
#include <QProcess>

#if defined(Q_OS_WIN32)
#include <windows.h>
#endif

namespace atools {
namespace fs {

const static QChar SEP(QDir::separator());

QHash<atools::fs::FsPaths::SimulatorType, QString> FsPaths::basePathMap;
QHash<atools::fs::FsPaths::SimulatorType, QString> FsPaths::filesPathMap;
QHash<atools::fs::FsPaths::SimulatorType, QString> FsPaths::sceneryFilepathMap;

static const QVector<atools::fs::FsPaths::SimulatorType> ALL_SIMULATOR_TYPES(
  {
    FsPaths::FSX, FsPaths::FSX_SE, FsPaths::P3D_V2, FsPaths::P3D_V3, FsPaths::P3D_V4, FsPaths::P3D_V5,
    FsPaths::XPLANE11, FsPaths::MSFS
  });

static const QHash<atools::fs::FsPaths::SimulatorType, QString> ALL_SIMULATOR_TYPE_NAMES(
  {
    {FsPaths::FSX, "FSX"},
    {FsPaths::FSX_SE, "FSXSE"},
    {FsPaths::P3D_V2, "P3DV2"},
    {FsPaths::P3D_V3, "P3DV3"},
    {FsPaths::P3D_V4, "P3DV4"},
    {FsPaths::P3D_V5, "P3DV5"},
    {FsPaths::XPLANE11, "XP11"},
    {FsPaths::MSFS, "MSFS"},
    {FsPaths::NAVIGRAPH, "NAVIGRAPH"}
  });

static const QHash<atools::fs::FsPaths::SimulatorType, QString> ALL_SIMULATOR_NAMES(
  {
    {FsPaths::FSX, "Microsoft Flight Simulator X"},
    {FsPaths::FSX_SE, "Microsoft Flight Simulator - Steam Edition"},
    {FsPaths::P3D_V2, "Prepar3D v2"},
    {FsPaths::P3D_V3, "Prepar3D v3"},
    {FsPaths::P3D_V4, "Prepar3D v4"},
    {FsPaths::P3D_V5, "Prepar3D v5"},
    {FsPaths::XPLANE11, "X-Plane 11"},
    {FsPaths::MSFS, "Microsoft Flight Simulator 2020"},
    {FsPaths::NAVIGRAPH, "Navigraph"}
  }
  );

/* Platform: FSX, FSX XPack, FSX Gold */
const QLatin1String FSX_REGISTRY_PATH("HKEY_CURRENT_USER\\Software\\Microsoft");
const QStringList FSX_REGISTRY_KEY = {"Microsoft Games", "Flight Simulator", "10.0", "AppPath"};

/* Platform: FSX Steam Edition */
const QLatin1String FSX_SE_REGISTRY_PATH("HKEY_CURRENT_USER\\Software\\Microsoft");
const QStringList FSX_SE_REGISTRY_KEY = {"Microsoft Games", "Flight Simulator - Steam Edition", "10.0", "AppPath"};

/* Platform: Prepar3d Version 2 */
const QLatin1String P3D_V2_REGISTRY_PATH("HKEY_CURRENT_USER\\Software");
const QStringList P3D_V2_REGISTRY_KEY = {"Lockheed Martin", "Prepar3D v2", "AppPath"};

/* Platform: Prepar3d Version 3 */
const QLatin1String P3D_V3_REGISTRY_PATH("HKEY_CURRENT_USER\\Software");
const QStringList P3D_V3_REGISTRY_KEY = {"Lockheed Martin", "Prepar3D v3", "AppPath"};

/* Platform: Prepar3d Version 4 */
const QLatin1String P3D_V4_REGISTRY_PATH("HKEY_CURRENT_USER\\Software");
const QStringList P3D_V4_REGISTRY_KEY = {"Lockheed Martin", "Prepar3D v4", "AppPath"};

/* Platform: Prepar3d Version 5 */
const QLatin1String P3D_V5_REGISTRY_PATH("HKEY_CURRENT_USER\\Software");
const QStringList P3D_V5_REGISTRY_KEY = {"Lockheed Martin", "Prepar3D v5", "AppPath"};

/* Settings keys */
const QLatin1String SETTINGS_FSX_PATH("FsPaths/FsxPath");
const QLatin1String SETTINGS_FSX_SE_PATH("FsPaths/FsxSePath");
const QLatin1String SETTINGS_P3D_V2_PATH("FsPaths/P3dV2Path");
const QLatin1String SETTINGS_P3D_V3_PATH("FsPaths/P3dV3Path");
const QLatin1String SETTINGS_P3D_V4_PATH("FsPaths/P3dV4Path");
const QLatin1String SETTINGS_P3D_V5_PATH("FsPaths/P3dV5Path");
const QLatin1String SETTINGS_XPLANE11_PATH("FsPaths/XPlane11Path");
const QLatin1String SETTINGS_MSFS_PATH("FsPaths/MsfsPath");

/* Paths for non Windows systems - used for development and debugging purposes */
const QLatin1String FSX_NO_WINDOWS_PATH("Microsoft Flight Simulator X");
const QLatin1String FSX_SE_NO_WINDOWS_PATH("Flight Simulator - Steam Edition");
const QLatin1String P3D_V2_NO_WINDOWS_PATH("Prepar3D v2");
const QLatin1String P3D_V3_NO_WINDOWS_PATH("Prepar3D v3");
const QLatin1String P3D_V4_NO_WINDOWS_PATH("Prepar3D v4");
const QLatin1String P3D_V5_NO_WINDOWS_PATH("Prepar3D v5");
const QLatin1String MSFS_NO_WINDOWS_PATH("MSFS2020");

QProcessEnvironment environment;

using atools::settings::Settings;

void FsPaths::logAllPaths()
{
  qDebug() << Q_FUNC_INFO;

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

    qInfo().nospace().noquote() << ALL_SIMULATOR_TYPE_NAMES[type] << " - " << ALL_SIMULATOR_NAMES[type];
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
  }
}

void FsPaths::loadAllPaths()
{
  qDebug() << Q_FUNC_INFO;

  basePathMap.clear();
  filesPathMap.clear();
  sceneryFilepathMap.clear();

  for(atools::fs::FsPaths::SimulatorType type : ALL_SIMULATOR_TYPES)
  {
    basePathMap.insert(type, initBasePath(type));
    filesPathMap.insert(type, initFilesPath(type));
    sceneryFilepathMap.insert(type, initSceneryLibraryPath(type));
  }
}

void FsPaths::intitialize()
{
  qRegisterMetaTypeStreamOperators<atools::fs::FsPaths::SimulatorType>();
  environment = QProcessEnvironment::systemEnvironment();
}

QString FsPaths::initBasePath(SimulatorType type)
{
  QString fsPath;
  if(type == NAVIGRAPH || type == UNKNOWN)
    return QString();

  if(type == XPLANE11)
  {
    // =====================================================
    // The location of this file varies by operating system:
    // OS X - the file will be in the user’s preferences folder, e.g. ~/Library/Preferences/.
    // Windows -  the file will be in the user’s local app data folder, e.g. C:\Users\nnnn\AppData\Local\.
    // Use CSIDL_LOCAL_APPDATA to find the location.
    // Linux - the file will be in the user’s home folder in a .x-plane/ sub folder, e.g. ~/.x-plane/.

#if defined(Q_OS_WIN32)
    // "C:\Users\USERS\AppData\Local\x-plane_install_11.txt"
    return xplaneBasePath(environment.value("LOCALAPPDATA") + SEP + "x-plane_install_11.txt");

#elif defined(Q_OS_MACOS)
    // "/Users/USER/Library/Preferences/x-plane_install_11.txt"
    return validXplaneBasePath(
             QDir::homePath() + SEP +
             "Library" + SEP +
             "Preferences" + SEP +
             "x-plane_install_11.txt");

#elif defined(Q_OS_LINUX)
    // "/home/USER/.x-plane/x-plane_install_11.txt"
    return xplaneBasePath(
             QDir::homePath() + SEP + ".x-plane" + SEP + "x-plane_install_11.txt");

#endif
  }
  else if(type == MSFS)
  {
    // Read UserCfg.opt to find the packages installation path
#if defined(Q_OS_WIN32)
    // Fixed location
    // C:\Users\USER\AppData\Local\Packages\Microsoft.FlightSimulator_8wekyb3d8bbwe\LocalCache\UserCfg.opt
    fsPath = msfsBasePath(environment.value("LOCALAPPDATA") + SEP +
                          "Packages" + SEP +
                          "Microsoft.FlightSimulator_8wekyb3d8bbwe" + SEP +
                          "LocalCache" + SEP + "UserCfg.opt");

    // C:\Users\USER\AppData\Roaming\Microsoft Flight Simulator\UserCfg.opt
    if(fsPath.isEmpty())
      fsPath = msfsBasePath(environment.value("APPDATA") + SEP +
                            "Microsoft Flight Simulator" + SEP + "UserCfg.opt");

#elif defined(DEBUG_FS_PATHS)

    QString nonWinPath = nonWindowsPathFull(type);

    // /home/alex/Simulators/MSFS2020\Packages\Microsoft.FlightSimulator_8wekyb3d8bbwe\LocalCache\UserCfg.opt
    fsPath = msfsBasePath(nonWinPath + SEP + "Packages" + SEP +
                          "Microsoft.FlightSimulator_8wekyb3d8bbwe" + SEP + "LocalCache" + SEP +
                          "UserCfg.opt");

    if(fsPath.isEmpty())
      fsPath = msfsBasePath(nonWinPath + SEP + "Microsoft Flight Simulator" + SEP + "UserCfg.opt");
#endif
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

    if(found && !keys.isEmpty() && settings.contains(keys.last()))
    {
      fsPath = settings.value(keys.last()).toString();

      if(fsPath.endsWith('\\'))
        fsPath.chop(1);
    }
#elif defined(DEBUG_FS_PATHS)
    // No Windows here - get the path for debugging purposes =====================================================
    // from the configuration file
    fsPath = nonWindowsPathFull(type);
#endif

    if(!fsPath.isEmpty())
    {
      QFileInfo basePathInfo(fsPath);
      if(!basePathInfo.exists() || !basePathInfo.isDir())
      {
        qWarning() << Q_FUNC_INFO << "Path does not exist or is not a directory" << fsPath;
        fsPath.clear();
      }
    }
    else
      qWarning() << Q_FUNC_INFO << "Path is empty";

    // qDebug() << "Found a flight simulator base path for type" << type << "at" << fsPath;
  }
  return fsPath;
}

QString FsPaths::msfsSimPath()
{
  // C:\Users\USER\AppData\Local\Packages\Microsoft.FlightSimulator_8wekyb3d8bbwe
  QString fsPath = environment.value("LOCALAPPDATA") + SEP + "Packages" + SEP +
                   "Microsoft.FlightSimulator_8wekyb3d8bbwe";

  // C:\Users\USER\AppData\Roaming\Microsoft Flight Simulator
  if(!atools::checkDir(fsPath).isEmpty())
    fsPath = environment.value("APPDATA") + SEP + "Microsoft Flight Simulator";

  if(atools::checkDir(fsPath).isEmpty())
    return fsPath;
  else
    return QString();
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
      QString home = QDir::homePath() + SEP + QString("Simulators");
      QString nonWinPath = nonWindowsPath(type);

      if(!nonWinPath.isEmpty())
      {
        QFileInfo fi(home + SEP + nonWinPath);
        if(fi.exists() && fi.isDir() && fi.isReadable())
          fsPath = fi.absoluteFilePath();
      }
    }
  }
  return fsPath;
}

bool FsPaths::hasSim(FsPaths::SimulatorType type)
{
  return type == XPLANE11 ? true : !getBasePath(type).isEmpty();
}

QString FsPaths::initFilesPath(SimulatorType type)
{
  QString fsFilesDir;

  switch(type)
  {
    case atools::fs::FsPaths::XPLANE11:
      fsFilesDir = atools::buildPathNoCase({getBasePath(type), "Output", "FMS Plans"});
      break;

    case atools::fs::FsPaths::MSFS:
      // C:\Users\USER\AppData\Local\Packages\Microsoft.FlightSimulator_8wekyb3d8bbwe\LocalState
      // C:\Users\USER\AppData\Roaming\Microsoft Flight Simulator\LocalState ?
      fsFilesDir = msfsSimPath();
      break;

    case atools::fs::FsPaths::FSX:
    case atools::fs::FsPaths::FSX_SE:
    case atools::fs::FsPaths::P3D_V2:
    case atools::fs::FsPaths::P3D_V3:
    case atools::fs::FsPaths::P3D_V4:
    case atools::fs::FsPaths::P3D_V5:
#if defined(Q_OS_WIN32)
      {
        QString languageDll(getBasePath(type) + SEP + "language.dll");
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
            QFileInfo fsFilesDirInfo(document + SEP + QString::fromWCharArray(filesPathWChar));
            if(fsFilesDirInfo.exists() && fsFilesDirInfo.isDir() && fsFilesDirInfo.isReadable())
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
#endif
      break;

    case atools::fs::FsPaths::NAVIGRAPH:
    case atools::fs::FsPaths::ALL_SIMULATORS:
    case atools::fs::FsPaths::UNKNOWN:
      break;

  }

  // Use fallback on non Windows systems or if not found
  if(fsFilesDir.isEmpty())
    fsFilesDir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();

  return fsFilesDir;
}

QString FsPaths::initSceneryLibraryPath(SimulatorType type)
{
#if defined(Q_OS_WIN32)
  // Win 7+ C:\ProgramData
  QString programData(environment.value("PROGRAMDATA"));

  // Win 7+ C:\Users\{username}\AppData\Roaming
  QString appData(environment.value("APPDATA"));

  QString allUsersProfile(environment.value("ALLUSERSPROFILE"));

  if(programData.isEmpty())
    programData = allUsersProfile + SEP + QDir(appData).dirName();
#endif

  switch(type)
  {
    case FSX:
      // FSX C:\Users\user account name\AppData\Roaming\Microsoft\FSX\scenery.cfg
      // or C:\ProgramData\Microsoft\FSX\Scenery.cfg
#if defined(Q_OS_WIN32)
      return programData + SEP + "Microsoft\\FSX\\Scenery.CFG";

#elif defined(DEBUG_FS_PATHS)
      return getBasePath(type) + SEP + "scenery.cfg";

#endif

    case FSX_SE:
      // FSX SE C:\ProgramData\Microsoft\FSX-SE\Scenery.cfg
#if defined(Q_OS_WIN32)
      return programData + SEP + "Microsoft\\FSX-SE\\Scenery.CFG";

#elif defined(DEBUG_FS_PATHS)
      return getBasePath(type) + SEP + "scenery.cfg";

#endif

    case P3D_V2:
      // P3D v2 C:\Users\user account name\AppData\Roaming\Lockheed Martin\Prepar3D v2
#if defined(Q_OS_WIN32)
      return appData + SEP + "Lockheed Martin\\Prepar3D v2\\Scenery.CFG";

#elif defined(DEBUG_FS_PATHS)
      return getBasePath(type) + SEP + "scenery.cfg";

#endif

    case P3D_V3:
      // P3D v3 C:\ProgramData\Lockheed Martin\Prepar3D v3
#if defined(Q_OS_WIN32)
      return programData + SEP + "Lockheed Martin\\Prepar3D v3\\Scenery.CFG";

#elif defined(DEBUG_FS_PATHS)
      return getBasePath(type) + SEP + "scenery.cfg";

#endif

    case P3D_V4:
      // P3D v4 C:\ProgramData\Lockheed Martin\Prepar3D v4
#if defined(Q_OS_WIN32)
      return programData + SEP + "Lockheed Martin\\Prepar3D v4\\Scenery.CFG";

#elif defined(DEBUG_FS_PATHS)
      return getBasePath(type) + SEP + "scenery.cfg";

#endif

    case P3D_V5:
      // P3D v5 C:\ProgramData\Lockheed Martin\Prepar3D v5
#if defined(Q_OS_WIN32)
      return programData + SEP + "Lockheed Martin\\Prepar3D v5\\Scenery.CFG";

#elif defined(DEBUG_FS_PATHS)
      return getBasePath(type) + SEP + "scenery.cfg";

#endif

    // Disable compiler warnings - simulators that dont have a file reference
    case MSFS:
    case XPLANE11:
    case NAVIGRAPH:
    case UNKNOWN:
    case ALL_SIMULATORS:
      break;
  }
  return QString();
}

QString FsPaths::typeToShortName(SimulatorType type)
{
  return ALL_SIMULATOR_TYPE_NAMES.value(type);
}

QString FsPaths::typeToName(SimulatorType type)
{
  return ALL_SIMULATOR_NAMES.value(type);
}

FsPaths::SimulatorType FsPaths::stringToType(const QString& typeStr)
{
  QString type = typeStr.toUpper();
  if(type == "FSX")
    return FSX;
  else if(type == "FSXSE")
    return FSX_SE;
  else if(type == "P3DV2")
    return P3D_V2;
  else if(type == "P3DV3")
    return P3D_V3;
  else if(type == "P3DV4")
    return P3D_V4;
  else if(type == "P3DV5")
    return P3D_V5;
  else if(type == "XP11")
    return XPLANE11;
  else if(type == "MSFS")
    return MSFS;
  else if(type == "NAVIGRAPH" || type == "DFD")
    return NAVIGRAPH;
  else
    return UNKNOWN;
}

const QVector<FsPaths::SimulatorType>& FsPaths::getAllSimulatorTypes()
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

    case P3D_V2:
      return SETTINGS_P3D_V2_PATH;

    case P3D_V3:
      return SETTINGS_P3D_V3_PATH;

    case P3D_V4:
      return SETTINGS_P3D_V4_PATH;

    case P3D_V5:
      return SETTINGS_P3D_V5_PATH;

    case XPLANE11:
      return SETTINGS_XPLANE11_PATH;

    case MSFS:
      return SETTINGS_MSFS_PATH;

    case NAVIGRAPH:
    case UNKNOWN:
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

    case P3D_V2:
      return P3D_V2_REGISTRY_PATH;

    case P3D_V3:
      return P3D_V3_REGISTRY_PATH;

    case P3D_V4:
      return P3D_V4_REGISTRY_PATH;

    case P3D_V5:
      return P3D_V5_REGISTRY_PATH;

    case MSFS:
    case XPLANE11:
    case NAVIGRAPH:
    case UNKNOWN:
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

    case P3D_V2:
      return P3D_V2_REGISTRY_KEY;

    case P3D_V3:
      return P3D_V3_REGISTRY_KEY;

    case P3D_V4:
      return P3D_V4_REGISTRY_KEY;

    case P3D_V5:
      return P3D_V5_REGISTRY_KEY;

    case MSFS:
    case XPLANE11:
    case NAVIGRAPH:
    case ALL_SIMULATORS:
    case UNKNOWN:
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

    case P3D_V2:
      return P3D_V2_NO_WINDOWS_PATH;

    case P3D_V3:
      return P3D_V3_NO_WINDOWS_PATH;

    case P3D_V4:
      return P3D_V4_NO_WINDOWS_PATH;

    case P3D_V5:
      return P3D_V5_NO_WINDOWS_PATH;

    case MSFS:
      return MSFS_NO_WINDOWS_PATH;

    case XPLANE11:
    case NAVIGRAPH:
    case UNKNOWN:
    case ALL_SIMULATORS:
      break;
  }
  return QString();
}

QString FsPaths::msfsBasePath(const QString& userCfgOptFile)
{
  QString dir;
  QFile fileCfgOpt(userCfgOptFile);
  if(fileCfgOpt.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QTextStream stream(&fileCfgOpt);

    while(!stream.atEnd())
    {
      QString line = stream.readLine().trimmed();

      if(line.startsWith("InstalledPackagesPath"))
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
  }
  else
    qWarning() << Q_FUNC_INFO << "Cannot open" << userCfgOptFile << "error" << fileCfgOpt.errorString();

  return dir;
}

QString FsPaths::xplaneBasePath(const QString& installationFile)
{
  QString dir;
  QFile file(installationFile);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QTextStream stream(&file);

    while(!stream.atEnd())
    {
      QFileInfo fi(stream.readLine().trimmed());
      if(fi.exists() && fi.isDir())
      {
        dir = fi.absoluteFilePath();
        break;
      }
      else
        qWarning() << Q_FUNC_INFO << fi.absoluteFilePath() << "does not exist or is not a directory";
    }

    file.close();
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
