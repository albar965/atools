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

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

#if defined(Q_OS_WIN32)
#include <windows.h>
#endif

namespace atools {
namespace fs {

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
  fsPath = s->value(settingsKey(type)).toString();
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

QString FsPaths::getFilesPath(SimulatorType type)
{
  QString fsFilesDir;

#if defined(Q_OS_WIN32)
  QString languageDll(getBasePath(type) + QDir::separator() + "language.dll");
  qDebug() << "Language DLL" << languageDll;

  // Copy to wchar and append null
  wchar_t languageDllWChar[languageDll.size() + 1];
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

    // Check all Documents folders for path - there should be only one
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
  // TODO implement getSceneryLibraryPath
  // scenery.cfg
  // FSX C:\Users\user account name\AppData\Roaming\Microsoft\FSX
  // P3D v2 C:\Users\user account name\AppData\Roaming\Lockheed Martin\Prepar3D v2
  // P3D v3 C:\ProgramData\Lockheed Martin\Prepar3D v3
  Q_UNUSED(type);
  return QString();
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
  }
  Q_ASSERT_X(false, "FsPaths", "Unknown SimulatorType");
  return nullptr;
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
  }
  Q_ASSERT_X(false, "FsPaths", "Unknown SimulatorType");
  return nullptr;
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
  }
  Q_ASSERT_X(false, "FsPaths", "Unknown SimulatorType");
  return nullptr;
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
  }
  Q_ASSERT_X(false, "FsPaths", "Unknown SimulatorType");
  return nullptr;
}

} /* namespace fs */
} /* namespace atools */
