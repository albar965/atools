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

#include "fspaths.h"

#include "settings/settings.h"
#include "logging/loggingdefs.h"

#include <QHash>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QDataStream>

#if defined(Q_OS_WIN32)
#include <windows.h>
#endif

namespace atools {
namespace fs {

const atools::fs::FsPaths::SimulatorType FsPaths::ALL_SIMULATOR_TYPES[NUM_SIMULATOR_TYPES] =
{FsPaths::FSX, FsPaths::FSX_SE, FsPaths::P3D_V2, FsPaths::P3D_V3};

const QString ALL_SIMULATOR_TYPE_NAMES[NUM_SIMULATOR_TYPES] = {"FSX", "FSXSE", "P3DV2", "P3DV3"};

const QString ALL_SIMULATOR_NAMES[NUM_SIMULATOR_TYPES] =
{
  "Microsoft Flight Simulator X", "Flight Simulator - Steam Edition", "Prepar3D v2", "Prepar3D v3"
};

const char *FsPaths::FSX_REGISTRY_PATH =
  "HKEY_CURRENT_USER\\Software\\Microsoft\\Microsoft Games\\Flight Simulator\\10.0";
const char *FsPaths::FSX_REGISTRY_KEY = "AppPath";

const char *FsPaths::FSX_SE_REGISTRY_PATH =
  "HKEY_CURRENT_USER\\Software\\Microsoft\\Microsoft Games\\Flight Simulator - Steam Edition\\10.0";
const char *FsPaths::FSX_SE_REGISTRY_KEY = "AppPath";

const char *FsPaths::P3D_V2_REGISTRY_PATH =
  "HKEY_CURRENT_USER\\Software\\Lockheed Martin\\Prepar3D v2";
const char *FsPaths::P3D_V2_REGISTRY_KEY = "AppPath";

const char *FsPaths::P3D_V3_REGISTRY_PATH =
  "HKEY_CURRENT_USER\\Software\\Lockheed Martin\\Prepar3D v3";
const char *FsPaths::P3D_V3_REGISTRY_KEY = "AppPath";

const char *FsPaths::SETTINGS_FSX_PATH = "FsPaths/FsxPath";
const char *FsPaths::SETTINGS_FSX_SE_PATH = "FsPaths/FsxSePath";
const char *FsPaths::SETTINGS_P3D_V2_PATH = "FsPaths/P3dV2Path";
const char *FsPaths::SETTINGS_P3D_V3_PATH = "FsPaths/P3dV3Path";

const char *FsPaths::FSX_NO_WINDOWS_PATH = "Microsoft Flight Simulator X";
const char *FsPaths::FSX_SE_NO_WINDOWS_PATH = "Flight Simulator - Steam Edition";
const char *FsPaths::P3D_V2_NO_WINDOWS_PATH = "Prepar3D v2";
const char *FsPaths::P3D_V3_NO_WINDOWS_PATH = "Prepar3D v3";

using atools::settings::Settings;

QString FsPaths::getBasePath(SimulatorType type)
{
  QString fsPath;
#if defined(Q_OS_WIN32)
  // Try to get the FSX path from the Windows registry
  QSettings settings(registryPath(type), QSettings::NativeFormat);
  fsPath = settings.value(registryKey(type)).toString();
  if(fsPath.endsWith('\\'))
    fsPath.chop(1);
#else
  // No Windows here - get the path for debugging purposes
  // from the configuration file
  Settings& s = Settings::instance();
  fsPath = s.valueStr(settingsKey(type));
  if(fsPath.isEmpty())
  {
    // If it is not present in the settings file use one of the predefined paths
    // Useful with symlinks for debugging
    QString home = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
    QFileInfo fi(home + QDir::separator() + nonWindowsPath(type));
    if(fi.exists() && fi.isDir() && fi.isReadable())
      fsPath = fi.absoluteFilePath();
  }
#endif

  qDebug() << "Found a flight simulator base path for type" << type << "at" << fsPath;

  return fsPath;
}

bool FsPaths::hasSim(FsPaths::SimulatorType type)
{
  return !getBasePath(type).isEmpty();
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
#else
  // Use fallback on non Windows systems
  if(fsFilesDir.isEmpty())
  {
    qDebug() << "Using fallback to find flight simulator documents path";
    fsFilesDir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).at(0);
  }
#endif
  qDebug() << "Found a flight simulator documents path for type" << type << "at" << fsFilesDir;

  return fsFilesDir;
}

QString FsPaths::getSceneryLibraryPath(SimulatorType type)
{
  // scenery.cfg
  // FSX C:\Users\user account name\AppData\Roaming\Microsoft\FSX
  // or C:\ProgramData\Microsoft\FSX\Scenery.cfg
  // FSX SE C:\ProgramData\Microsoft\FSX-SE\Scenery.cfg
  // P3D v2 C:\Users\user account name\AppData\Roaming\Lockheed Martin\Prepar3D v2
  // P3D v3 C:\ProgramData\Lockheed Martin\Prepar3D v3
  QString home = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
  switch(type)
  {
    case FSX:
#if defined(Q_OS_WIN32)
      // Windows 7 and above
      // return "C:\\ProgramData\\Microsoft\\FSX\\Scenery.CFG";

      // Windows XP and above
      return "c:\\Documents And Settings\\All Users\\Application Data\\Microsoft\\FSX\\Scenery.CFG";

#else
      return home + QDir::separator() +
             "Temp" + QDir::separator() +
             "FSX" + QDir::separator() + "scenery.cfg";

#endif

    case FSX_SE:
#if defined(Q_OS_WIN32)
      return "C:\\ProgramData\\Microsoft\\FSX-SE\\Scenery.CFG";

#else
      return home + QDir::separator() +
             "Temp" + QDir::separator() +
             "FSXSE" + QDir::separator() + "scenery.cfg";

#endif

    case P3D_V2:
#if defined(Q_OS_WIN32)
      return home + QDir::separator() +
             "AppData" + QDir::separator() +
             "Roaming" + QDir::separator() +
             "Lockheed Martin" + QDir::separator() +
             "Prepar3D v2" + QDir::separator() + "Scenery.CFG";

#else
      return home + QDir::separator() +
             "Temp" + QDir::separator() +
             "P3DV3" + QDir::separator() + "scenery.cfg";

#endif

    case P3D_V3:
#if defined(Q_OS_WIN32)
      return "C:\\ProgramData\\Lockheed Martin\\Prepar3D v3\\Scenery.CFG";

#else
      return home + QDir::separator() +
             "Temp" + QDir::separator() +
             "P3DV3" + QDir::separator() + "scenery.cfg";

#endif
    case UNKNOWN:
    case MAX_VALUE:
    case ALL_SIMULATORS:
      break;
  }
  return QString();
}

QString FsPaths::typeToShortName(SimulatorType type)
{
  if(type >= FSX && type < MAX_VALUE)
    return ALL_SIMULATOR_TYPE_NAMES[type];
  else
    return QString();
}

QString FsPaths::typeToName(SimulatorType type)
{
  if(type >= FSX && type < MAX_VALUE)
    return ALL_SIMULATOR_NAMES[type];
  else
    return QString();
}

FsPaths::SimulatorType FsPaths::stringToType(const QString& typeStr)
{
  if(typeStr == "FSX")
    return FSX;
  else if(typeStr == "FSXSE")
    return FSX_SE;
  else if(typeStr == "P3DV2")
    return P3D_V2;
  else if(typeStr == "P3DV3")
    return P3D_V3;
  else
    return UNKNOWN;
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

    case UNKNOWN:
    case MAX_VALUE:
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

    case UNKNOWN:
    case MAX_VALUE:
    case ALL_SIMULATORS:
      break;
  }
  return QString();
}

QString FsPaths::registryKey(SimulatorType type)
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

    case MAX_VALUE:
    case ALL_SIMULATORS:
    case UNKNOWN:
      break;
  }
  return QString();
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

    case UNKNOWN:
    case MAX_VALUE:
    case ALL_SIMULATORS:
      break;
  }
  return QString();
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
