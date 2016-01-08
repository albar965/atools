#ifndef FSPATHS_H
#define FSPATHS_H

#include <QString>

namespace atools {
namespace fs {

enum SimulatorType
{
  /* Platform: FSX, FSX XPack, FSX Gold */
  FSX,

  /* Platform: FSX Steam Edition */
  FSX_SE,

  /* Platform: Prepar3d Version 2 */
  P3D_V2,

  /* Platform: Prepar3d Version 3 */
  P3D_V3
};

class FsPaths
{
public:
  static QString getBasePath(SimulatorType type);

  static QString getDocumentsPath(SimulatorType type);

  static QString getSceneryLibraryPath(SimulatorType type);

private:
  FsPaths()
  {

  }

  /* registry path and key if running on Windows */
  /* Platform: FSX, FSX XPack, FSX Gold */
  static const char *FSX_REGISTRY_PATH;
  static const char *FSX_REGISTRY_KEY;

  /* Platform: FSX Steam Edition */
  static const char *FSX_SE_REGISTRY_PATH;
  static const char *FSX_SE_REGISTRY_KEY;

  /* Platform: Prepar3d Version 2 */
  static const char *P3D_V2_REGISTRY_PATH;
  static const char *P3D_V2_REGISTRY_KEY;

  /* Platform: Prepar3d Version 3 */
  static const char *P3D_V3_REGISTRY_PATH;
  static const char *P3D_V3_REGISTRY_KEY;

  /* Use this as fallback from the settings if not running on Windows */
  static const char *SETTINGS_FSX_PATH;
  static const char *SETTINGS_FSX_SE_PATH;
  static const char *SETTINGS_P3D_V2_PATH;
  static const char *SETTINGS_P3D_V3_PATH;

  QString documentsDirectory(QString simBasePath);

  static const char *settingsKey(SimulatorType type);
  static const char *registryPath(SimulatorType type);
  static const char *registryKey(SimulatorType type);

  static QStringList findFsxDocuments();
};

} /* namespace fs */
} /* namespace atools */

#endif // FSPATHS_H
