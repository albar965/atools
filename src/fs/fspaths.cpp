#include "fspaths.h"

#include "settings/settings.h"
#include "logging/loggingdefs.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

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

// scenery.cfg
// FSX C:\Users\user account name\AppData\Roaming\Microsoft\FSX
// P3D v2 C:\Users\user account name\AppData\Roaming\Lockheed Martin\Prepar3D v2
// P3D v3 C:\ProgramData\Lockheed Martin\Prepar3D v3

using atools::settings::Settings;

QString FsPaths::getBasePath(atools::fs::SimulatorType type)
{
  QString fsxPath;
#if defined(Q_OS_WIN32)
  // Try to get the FSX path from the Windows registry
  QSettings settings(registryPath(type), QSettings::NativeFormat);
  fsxPath = settings.value(registryKey(type)).toString();
#else
  // No Windows here - get the path for debugging purposes
  // from the configuration file
  Settings& s = Settings::instance();
  fsxPath = s->value(settingsKey(type)).toString();
#endif

  qDebug() << "Found a flight simulator base path for type" << type << "at" << fsxPath;

  return fsxPath;
}

QString FsPaths::getDocumentsPath(SimulatorType type)
{
  QString simBasePath = getBasePath(type);
  QString docPath;

#if defined(Q_OS_WIN32)
  QByteArray langDll = QString(simBasePath + "\\language.dll").toLocal8Bit();

  // char dll_path[MAX_PATH];
  // assuming FS_Path is a string that contains the path to the FS installation...
  // strcpy_s(dll_path, MAX_PATH - 1, FS_Path);
  // strcat_s(dll_path, MAX_PATH - 1, "\\language.dll");

  char filesPath[1024];
  HINSTANCE hInstLang = LoadLibrary(langDll.data());
  if(hInstLang)
  {
    LoadStringA(hInstLang, 36864, filesPath, 1024);
    FreeLibrary(hInstLang);

    QFileInfo dir(QString::fromLocal8Bit(filesPath));
    if(dir.exists() && dir.isDir() && dir.isReadable())
      docPath = dir.absoluteFilePath();
  }
#else
  Q_UNUSED(simBasePath);
#endif

  if(docPath.isEmpty())
  {
    QStringList docDirs = findFsxDocuments();
    if(!docDirs.isEmpty())
      docPath = docDirs.at(0);
  }

  qDebug() << "Found a flight simulator documents path for type" << type << "at" << docPath;

  return docPath;
}

QString FsPaths::getSceneryLibraryPath(SimulatorType type)
{
  // TODO implement getSceneryLibraryPath
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
}

QStringList FsPaths::findFsxDocuments()
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

  return found;
}

} /* namespace fs */
} /* namespace atools */
