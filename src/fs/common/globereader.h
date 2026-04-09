/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_DTM_GLOBEREADER_H
#define ATOOLS_DTM_GLOBEREADER_H

#include <QCache>
#include <QFile>
#include <QList>

class DtmTest;
class QFileInfo;

namespace atools {
namespace geo {
class Pos;
class LineString;
}

namespace fs {
namespace common {

const float ELEVATION_INVALID = std::numeric_limits<float>::max();
const float ELEVATION_OCEAN = -500.f;

/*
 * DTM reader class for the GLOBE data which can be get at https://www.ngdc.noaa.gov/mgg/topo/globeget.html
 * https://www.ngdc.noaa.gov/mgg/topo/report/globedocumentationmanual.pdf
 *
 * Latitude-longitude grid spacing is 30 arc-seconds.
 *
 * Files ?10G and ?10B ("?" is the wildcard notation for tile letters "A" through "P") are provided as
 * 16-bit signed integer data in a simple binary raster using LittleEndian.
 * There are no header or trailer bytes embedded in the image.
 * The data is stored in row major order (all the data for row 1, followed by all the data for
 * row 2, etc.). All files have 10800 columns, and either 4800 or 6000 rows.
 */
class GlobeReader
{
public:
  explicit GlobeReader(const QString& dataDirParam);
  virtual ~GlobeReader();

  GlobeReader(const GlobeReader& other) = delete;
  GlobeReader& operator=(const GlobeReader& other) = delete;

  /* Valid if at least one file with matching name and size was found. */
  static bool isDirValid(const QString& path);

  /* Collect file names (up to 16). Files are opened on demand */
  bool openFiles();
  void closeFiles();

  /* Elevation in meter. If "distanceMeter" is > 0 then the center and four points around will be checked too.
   * "sampleRadiusMeter" defines a rectangle where five points are sampled and the maximum is used.*/
  float getElevation(const atools::geo::Pos& pos, float sampleRadiusMeter = 0.f);

  /* Get elevations in meter along a great circle line. Will create a point every 500 meters and delete
   * consecutive ones with same elevation
   * "sampleRadiusMeter" defines a rectangle where five points are sampled and the maximum is used.*/
  void getElevations(geo::LineString& elevations, const atools::geo::LineString& linestring, float sampleRadiusMeter = 0.f,
                     bool precision = false);

  /* true if folder exists and files were found */
  bool isValid() const
  {
    return valid;
  }

  /* Maximum number of files to keep in cache. Size of each is 98.88 Mb to 123.60 Mb  */
  void setCacheMaxFiles(int newCacheMaxFiles)
  {
    cacheMaxFiles = newCacheMaxFiles;
  }

private:
  friend class ::DtmTest;
  friend qint64 calcFileOffsetFromColRow(int gridCol, int gridRow, int& fileIndex);
  friend qint64 calcFileOffset(double lonx, double laty, int& fileIndex);

  /* Test method */
  qint64 calcFileOffsetTest(double lonx, double laty, int& fileIndex);

  /* Source data parameters*/
  const static qint64 FILE_SIZE_SMALL = 103680000;
  const static qint64 FILE_SIZE_LARGE = 129600000;
  const static int TILE_COLUMNS = 10800;
  const static int TILE_ROWS_SMALL = 4800;
  const static int TILE_ROWS_LARGE = 6000;

  const static int NUM_DATAFILES = 16;
  const static int GRID_COLUMNS = 4 * TILE_COLUMNS;
  const static int GRID_ROWS = 2 * 6000 + 2 * 4800;

  static bool fileEntryValid(const QFileInfo& fileEntry);
  void closeFile(int i);
  void openFile(int i);
  float elevationFromIndexAndOffset(int fileIndex, qint64 fileOffset);
  float elevationMax(const atools::geo::Pos& pos, float sampleRadiusMeter);

  int cacheMaxFiles = 8;
  QCache<int, QByteArray> fileCache;

  QString dataDir;
  QList<QString> dataFilenames;
  QList<QFile *> dataFiles;

  bool valid = false;
};

} // namespace common
} // namespace fs
} // namespace atools

#endif // ATOOLS_DTM_GLOBEREADER_H
