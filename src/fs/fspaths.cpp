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

const char *FsPaths::SETTINGS_FSX_PATH = "File/FsxPath";
const char *FsPaths::SETTINGS_FSX_SE_PATH = "File/FsxSePath";
const char *FsPaths::SETTINGS_P3D_V2_PATH = "File/P3dV2Path";
const char *FsPaths::SETTINGS_P3D_V3_PATH = "File/P3dV3Path";

using atools::settings::Settings;

QString FsPaths::getBasePath(atools::fs::SimulatorType type)
{
  QString fsxPath;
#if defined(Q_OS_WIN32)
  // Try to get the FSX path from the Windows registry
  QSettings settings(registryPath(type), QSettings::NativeFormat);
  fsxPath = settings.value(registryKey(type)).toString();
  if(fsxPath.endsWith('\\'))
    fsxPath.chop(1);
#else
  // No Windows here - get the path for debugging purposes
  // from the configuration file
  Settings& s = Settings::instance();
  fsxPath = s->value(settingsKey(type)).toString();
#endif

  qDebug() << "Found a flight simulator base path for type" << type << "at" << fsxPath;

  return fsxPath;
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
    QStringList fsFilesDirs = findFsxFiles();
    if(!fsFilesDirs.isEmpty())
      fsFilesDir = fsFilesDirs.at(0);
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

const char *FsPaths::settingsKey(SimulatorType type)
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

const char *FsPaths::registryPath(SimulatorType type)
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

const char *FsPaths::registryKey(SimulatorType type)
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

QStringList FsPaths::findFsxFiles()
{
  QStringList documents = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
  QStringList found;

  // Walk throug all document paths
  for(QString documentDir : documents)
  {
    QFileInfo dir(documentDir);

    if(dir.exists() && dir.isReadable() && dir.isDir())
    {
      // Directory "Documents" or similar

      // First check for the English directory
      QFileInfo lb(dir.absolutePath() + QDir::separator() + "Flight Simulator X Files");
      if(lb.exists() && lb.isReadable() && lb.isDir())
        found.append(lb.absoluteFilePath());

      // Try the German version
      lb = QFileInfo(dir.absolutePath() + QDir::separator() + "Flight Simulator X-Dateien");
      if(lb.exists() && lb.isReadable() && lb.isDir())
        found.append(lb.absoluteFilePath());

      // Find any directory matching the flight simulator name and add all
      // logbooks in these to a list
      QFileInfoList localDirs = QDir(dir.absoluteFilePath()).entryInfoList({"*Flight Simulator X*"},
                                                                           QDir::Dirs);

      for(QFileInfo localDir : localDirs)
        if(localDir.exists() && localDir.isReadable() && localDir.isDir())
          found.append(localDir.absoluteFilePath());
    }
  }

  qDebug() << "Found document paths using fallback:";
  for(QString doc : documents)
    qDebug() << doc;

  return found;
}

} /* namespace fs */
} /* namespace atools */
