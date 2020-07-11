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

static const QVector<atools::fs::FsPaths::SimulatorType> ALL_SIMULATOR_TYPES(
    {
      FsPaths::FSX, FsPaths::FSX_SE, FsPaths::P3D_V2, FsPaths::P3D_V3, FsPaths::P3D_V4, FsPaths::P3D_V5,
      FsPaths::XPLANE11
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
      {FsPaths::NAVIGRAPH, "NAVIGRAPH"}
    });

static const QHash<atools::fs::FsPaths::SimulatorType, QString> ALL_SIMULATOR_NAMES(
    {
      {FsPaths::FSX, "Microsoft Flight Simulator X"},
      {FsPaths::FSX_SE, "Flight Simulator - Steam Edition"},
      {FsPaths::P3D_V2, "Prepar3D v2"},
      {FsPaths::P3D_V3, "Prepar3D v3"},
      {FsPaths::P3D_V4, "Prepar3D v4"},
      {FsPaths::P3D_V5, "Prepar3D v5"},
      {FsPaths::XPLANE11, "X-Plane 11"},
      {FsPaths::NAVIGRAPH, "Navigraph"}
    }
  );

const char *FsPaths::FSX_REGISTRY_PATH = "HKEY_CURRENT_USER\\Software\\Microsoft";
const QStringList FsPaths::FSX_REGISTRY_KEY = {"Microsoft Games", "Flight Simulator", "10.0", "AppPath"};

const char *FsPaths::FSX_SE_REGISTRY_PATH = "HKEY_CURRENT_USER\\Software\\Microsoft";
const QStringList FsPaths::FSX_SE_REGISTRY_KEY =
{"Microsoft Games", "Flight Simulator - Steam Edition", "10.0", "AppPath"};

const char *FsPaths::P3D_V2_REGISTRY_PATH = "HKEY_CURRENT_USER\\Software";
const QStringList FsPaths::P3D_V2_REGISTRY_KEY = {"Lockheed Martin", "Prepar3D v2", "AppPath"};

const char *FsPaths::P3D_V3_REGISTRY_PATH = "HKEY_CURRENT_USER\\Software";
const QStringList FsPaths::P3D_V3_REGISTRY_KEY = {"Lockheed Martin", "Prepar3D v3", "AppPath"};

const char *FsPaths::P3D_V4_REGISTRY_PATH = "HKEY_CURRENT_USER\\Software";
const QStringList FsPaths::P3D_V4_REGISTRY_KEY = {"Lockheed Martin", "Prepar3D v4", "AppPath"};

const char *FsPaths::P3D_V5_REGISTRY_PATH = "HKEY_CURRENT_USER\\Software";
const QStringList FsPaths::P3D_V5_REGISTRY_KEY = {"Lockheed Martin", "Prepar3D v5", "AppPath"};

const char *FsPaths::SETTINGS_FSX_PATH = "FsPaths/FsxPath";
const char *FsPaths::SETTINGS_FSX_SE_PATH = "FsPaths/FsxSePath";
const char *FsPaths::SETTINGS_P3D_V2_PATH = "FsPaths/P3dV2Path";
const char *FsPaths::SETTINGS_P3D_V3_PATH = "FsPaths/P3dV3Path";
const char *FsPaths::SETTINGS_P3D_V4_PATH = "FsPaths/P3dV4Path";
const char *FsPaths::SETTINGS_P3D_V5_PATH = "FsPaths/P3dV5Path";
const char *FsPaths::SETTINGS_XPLANE11_PATH = "FsPaths/XPlane11Path";

const char *FsPaths::FSX_NO_WINDOWS_PATH = "Microsoft Flight Simulator X";
const char *FsPaths::FSX_SE_NO_WINDOWS_PATH = "Flight Simulator - Steam Edition";
const char *FsPaths::P3D_V2_NO_WINDOWS_PATH = "Prepar3D v2";
const char *FsPaths::P3D_V3_NO_WINDOWS_PATH = "Prepar3D v3";
const char *FsPaths::P3D_V4_NO_WINDOWS_PATH = "Prepar3D v4";
const char *FsPaths::P3D_V5_NO_WINDOWS_PATH = "Prepar3D v5";

QProcessEnvironment FsPaths::environment;

using atools::settings::Settings;

void FsPaths::logAllPaths()
{
  qInfo() << "Looking for flight simulator installations:";
  qInfo() << "PROGRAMDATA" << environment.value("PROGRAMDATA");
  qInfo() << "APPDATA" << environment.value("APPDATA");
  qInfo() << "LOCALAPPDATA" << environment.value("LOCALAPPDATA");
  qInfo() << "ALLUSERSPROFILE" << environment.value("ALLUSERSPROFILE");

  for(atools::fs::FsPaths::SimulatorType type : ALL_SIMULATOR_TYPES)
  {
    qInfo().nospace().noquote() << ALL_SIMULATOR_TYPE_NAMES[type] << " - " << ALL_SIMULATOR_NAMES[type];
    QString basePath = getBasePath(type);
    QString filesPath = getFilesPath(type);
    QString sceneryFilepath = getSceneryLibraryPath(type);

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

void FsPaths::intitialize()
{
  qRegisterMetaTypeStreamOperators<atools::fs::FsPaths::SimulatorType>();
  environment = QProcessEnvironment::systemEnvironment();
}

QString FsPaths::getBasePath(SimulatorType type)
{
  QString fsPath;
  if(type == NAVIGRAPH || type == UNKNOWN)
    return QString();

  if(type == XPLANE11)
  {
    // The location of this file varies by operating system:
    // OS X - the file will be in the user’s preferences folder, e.g. ~/Library/Preferences/.
    // Windows -  the file will be in the user’s local app data folder, e.g. C:\Users\nnnn\AppData\Local\.
    // Use CSIDL_LOCAL_APPDATA to find the location.
    // Linux - the file will be in the user’s home folder in a .x-plane/ sub folder, e.g. ~/.x-plane/.

#if defined(Q_OS_WIN32)
    // "C:\Users\USERS\AppData\Local\x-plane_install_11.txt"
    return validXplaneBasePath(environment.value("LOCALAPPDATA") + QDir::separator() + "x-plane_install_11.txt");

#elif defined(Q_OS_MACOS)
    // "/Users/USER/Library/Preferences/x-plane_install_11.txt"
    return validXplaneBasePath(
      QDir::homePath() + QDir::separator() +
      "Library" + QDir::separator() +
      "Preferences" + QDir::separator() +
      "x-plane_install_11.txt");

#elif defined(Q_OS_LINUX)
    // "/home/USER/.x-plane/x-plane_install_11.txt"
    return validXplaneBasePath(
      QDir::homePath() + QDir::separator() + ".x-plane" + QDir::separator() + "x-plane_install_11.txt");

#endif
  }
  else
  {
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
    // No Windows here - get the path for debugging purposes
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
        QString home = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0) + QDir::separator() +
                       QString("Simulators");
        QString nonWinPath = nonWindowsPath(type);

        if(!nonWinPath.isEmpty())
        {
          QFileInfo fi(home + QDir::separator() + nonWinPath);
          if(fi.exists() && fi.isDir() && fi.isReadable())
            fsPath = fi.absoluteFilePath();
        }
      }
    }
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

bool FsPaths::hasSim(FsPaths::SimulatorType type)
{
  return type == XPLANE11 ? true : !getBasePath(type).isEmpty();
}

QString FsPaths::getFilesPath(SimulatorType type)
{
  QString fsFilesDir;

#if defined(Q_OS_WIN32)
  QString languageDll(getBasePath(type) + QDir::separator() + "language.dll");
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
      QFileInfo fsFilesDirInfo(document + QDir::separator() + QString::fromWCharArray(filesPathWChar));
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
#endif

  // Use fallback on non Windows systems or if not found
  if(fsFilesDir.isEmpty())
  {
    qDebug() << "Using fallback to find flight simulator documents path for type" << type;
    fsFilesDir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
  }

  return fsFilesDir;
}

QString FsPaths::getSceneryLibraryPath(SimulatorType type)
{
#if defined(Q_OS_WIN32)
  // Win 7+ C:\ProgramData
  QString programData(environment.value("PROGRAMDATA"));

  // Win 7+ C:\Users\{username}\AppData\Roaming
  QString appData(environment.value("APPDATA"));

  QString allUsersProfile(environment.value("ALLUSERSPROFILE"));

  if(programData.isEmpty())
    programData = allUsersProfile + QDir::separator() + QDir(appData).dirName();
#else
  // If not windows use emulation for testing
  QString home = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
#endif

  switch(type)
  {
    case FSX:
      // FSX C:\Users\user account name\AppData\Roaming\Microsoft\FSX\scenery.cfg
      // or C:\ProgramData\Microsoft\FSX\Scenery.cfg
#if defined(Q_OS_WIN32)
      return programData + QDir::separator() + "Microsoft\\FSX\\Scenery.CFG";

#elif defined(DEBUG_FS_PATHS)
      return getBasePath(type) + QDir::separator() + "scenery.cfg";

#endif

    case FSX_SE:
      // FSX SE C:\ProgramData\Microsoft\FSX-SE\Scenery.cfg
#if defined(Q_OS_WIN32)
      return programData + QDir::separator() + "Microsoft\\FSX-SE\\Scenery.CFG";

#elif defined(DEBUG_FS_PATHS)
      return getBasePath(type) + QDir::separator() + "scenery.cfg";

#endif

    case P3D_V2:
      // P3D v2 C:\Users\user account name\AppData\Roaming\Lockheed Martin\Prepar3D v2
#if defined(Q_OS_WIN32)
      return appData + QDir::separator() + "Lockheed Martin\\Prepar3D v2\\Scenery.CFG";

#elif defined(DEBUG_FS_PATHS)
      return getBasePath(type) + QDir::separator() + "scenery.cfg";

#endif

    case P3D_V3:
      // P3D v3 C:\ProgramData\Lockheed Martin\Prepar3D v3
#if defined(Q_OS_WIN32)
      return programData + QDir::separator() + "Lockheed Martin\\Prepar3D v3\\Scenery.CFG";

#elif defined(DEBUG_FS_PATHS)
      return getBasePath(type) + QDir::separator() + "scenery.cfg";

#endif

    case P3D_V4:
      // P3D v4 C:\ProgramData\Lockheed Martin\Prepar3D v4
#if defined(Q_OS_WIN32)
      return programData + QDir::separator() + "Lockheed Martin\\Prepar3D v4\\Scenery.CFG";

#elif defined(DEBUG_FS_PATHS)
      return getBasePath(type) + QDir::separator() + "scenery.cfg";

#endif

    case P3D_V5:
      // P3D v5 C:\ProgramData\Lockheed Martin\Prepar3D v5
#if defined(Q_OS_WIN32)
      return programData + QDir::separator() + "Lockheed Martin\\Prepar3D v5\\Scenery.CFG";

#elif defined(DEBUG_FS_PATHS)
      return getBasePath(type) + QDir::separator() + "scenery.cfg";

#endif

    // Disable compiler warnings
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

    case XPLANE11:
    case NAVIGRAPH:
    case UNKNOWN:
    case ALL_SIMULATORS:
      break;
  }
  return QString();
}

QString FsPaths::validXplaneBasePath(const QString& installationFile)
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
