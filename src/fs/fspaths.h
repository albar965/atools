/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FS_FSPATHS_H
#define ATOOLS_FS_FSPATHS_H

#include <QObject>

namespace atools {
namespace fs {

/*
 * Allows to find Flight Simulator related paths and check for installed simulators.
 */
class FsPaths
{
  Q_GADGET

public:
  FsPaths() = delete;

  /* Print paths for all simulators to the info log channel */
  static void logAllPaths();

  /* Load and cache all paths */
  static void loadAllPaths();

  /* Register types and load process environment */
  static void intitialize();

  enum SimulatorType
  {
    /* Force numeric values since these are used as indexes.
     * Do not reorder and add new types at the end. */

    /* Platform: FSX, FSX XPack, FSX Gold */
    FSX = 0,

    /* Platform: FSX Steam Edition */
    FSX_SE = 1,

    /* Platform: Prepar3d Version 3 - mostly used for test purposes */
    P3D_V3 = 3,

    /* Platform: Prepar3d Version 4 */
    P3D_V4 = 6,

    /* Platform: Prepar3d Version 5 */
    P3D_V5 = 9,

    /* Platform: Prepar3d Version 6 */
    P3D_V6 = 12,

    /* X-Plane 11 */
    XPLANE_11 = 7,

    /* X-Plane 12 */
    XPLANE_12 = 11,

    /* Not a simulator but a database */
    NAVIGRAPH = 8,

    /* Synonym for database */
    DFD = NAVIGRAPH,

    /* Microsoft Flight Simulator 2020 */
    MSFS = 10,

    /* Microsoft Flight Simulator 2020 */
    MSFS_2024 = 13,

    /* Special value to pass to certain queries */
    ALL_SIMULATORS = -1,

    /* No simulator found */
    NONE = -2

    // Next is 14
  };

  Q_ENUM(SimulatorType)

  enum MsfsInstallType
  {
    MSFS_INSTALL_NONE,
    MSFS_INSTALL_ONLINE,
    MSFS_INSTALL_BOXED,
    MSFS_INSTALL_STEAM
  };

  Q_ENUM(MsfsInstallType)

  typedef QVector<atools::fs::FsPaths::SimulatorType> SimulatorTypeVector;

  /* Get installation path to fsx.exe, etc. Empty string if simulator is not installed.
   * Returns package installation path for MSFS. */
  static QString getBasePath(atools::fs::FsPaths::SimulatorType type);

  /* Get full path to language dependent "Flight Simulator X Files" or "Flight Simulator X-Dateien",
   * etc. Returns the documents path if FS files cannot be found. */
  static QString getFilesPath(atools::fs::FsPaths::SimulatorType type);

  /* Path to scenery.cfg for FSX/P3D. Empty for X-Plane and MSFS. */
  static QString getSceneryLibraryPath(atools::fs::FsPaths::SimulatorType type);

  /* Path to official MSFS scenery containing fs-base and fs-base-nav */
  static QString getMsfsOfficialPath();

  /* Default path. C:\Users\USERNAME\AppData\Local\Packages\Microsoft.Limitless_8wekyb3d8bbwe\LocalState\StreamedPackages\ */
  static QString getMsfs24StreamedPackagesPath();

  /* Detected installation type of MSFS */
  static MsfsInstallType getMsfsInstallType();
  static MsfsInstallType getMsfs24InstallType();

  /* Path to official folder for changed base where base is like
   * .../Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/Packages/Official/[OneStore|Steam]
   *  Checking for OneStore and Steam folders and layout files. Not used for MSFS 2024.
   * Empty if path not valid. */
  /* Path based on non-default path */
  static QString getMsfsOfficialPath(const QString& basePath);

  /* Path to community folder for MSFS. Default. */
  static QString getMsfsCommunityPath();

  /* Path to community folder for changed base where base is like
   * .../Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/Packages/Community .
   * Empty if path not valid. */
  /* Path based on non-default path */
  static QString getMsfsCommunityPath(const QString& basePath);

  /* Short abbreviated names for file reference but not user display. */
  static QString typeToShortName(atools::fs::FsPaths::SimulatorType type);

  /* return true if simulator can be found in the registry or base path was found */
  static bool hasSimulator(atools::fs::FsPaths::SimulatorType type);

  /* true if FSX, P3D or MSFS were found */
  static bool hasAnyMsSimulator();

  static bool hasAnyXplaneSimulator();
  static bool hasXplane11Simulator();
  static bool hasXplane12Simulator();

  /* X-Plane simulators */
  static bool isAnyXplane(atools::fs::FsPaths::SimulatorType type);

  /* Any Microsoft / SimConnect simulators */
  static bool isAnyMs(atools::fs::FsPaths::SimulatorType type);

  /* Long names like Microsoft Flight Simulator X */
  static QString typeToDisplayName(atools::fs::FsPaths::SimulatorType type);

  /* Short like X-Plane 11 or MSFS */
  static QString typeToShortDisplayName(atools::fs::FsPaths::SimulatorType type);

  static atools::fs::FsPaths::SimulatorType stringToType(const QString& typeStr);

  /* Array of all four valid types */
  static const QVector<atools::fs::FsPaths::SimulatorType>& getAllSimulatorTypes();

private:
  /* Get installation path to fsx.exe, etc. Empty string if simulator is not installed.
   * Returns package installation path for MSFS. */
  static QString initBasePath(atools::fs::FsPaths::SimulatorType type);

  /* Get full path to language dependent "Flight Simulator X Files" or "Flight Simulator X-Dateien",
   * etc. Returns the documents path if FS files cannot be found. */
  static QString initFilesPath(atools::fs::FsPaths::SimulatorType type);

  /* Path to scenery.cfg for FSX/P3D or Content.xml for MSFS. Empty for X-Plane. */
  static QString initSceneryLibraryPath(atools::fs::FsPaths::SimulatorType type);

  static QString settingsKey(atools::fs::FsPaths::SimulatorType type);
  static QString registryPath(atools::fs::FsPaths::SimulatorType type);
  static QStringList registryKey(atools::fs::FsPaths::SimulatorType type);

  static QString documentsDirectory(QString simBasePath);
  static QString nonWindowsPath(atools::fs::FsPaths::SimulatorType type);
  static QString xplaneBasePath(const QString& installationFile);
  static QString msfsBasePath(const QString& userCfgOptFile, atools::fs::FsPaths::SimulatorType type);
  static QString nonWindowsPathFull(atools::fs::FsPaths::SimulatorType type);

};

} /* namespace fs */
} /* namespace atools */

QDataStream& operator<<(QDataStream& out, const atools::fs::FsPaths::SimulatorType& obj);
QDataStream& operator>>(QDataStream& in, atools::fs::FsPaths::SimulatorType& obj);

#endif // ATOOLS_FS_FSPATHS_H
