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

#ifndef ATOOLS_DTM_GLOBEREADER_H
#define ATOOLS_DTM_GLOBEREADER_H

#include <QFile>
#include <QVector>

class DtmTest;
class QFileInfo;

namespace atools {
namespace geo {
class Pos;
class LineString;
}

namespace fs {
namespace common {

static Q_DECL_CONSTEXPR float INVALID = std::numeric_limits<float>::max();
static Q_DECL_CONSTEXPR float OCEAN = -500.f;

/*
 * DTM reader class for the GLOBE data which can be get at https://www.ngdc.noaa.gov/mgg/topo/globeget.html
 */
class GlobeReader
{
public:
  GlobeReader(const QString& dataDirParam);
  virtual ~GlobeReader();

  /* Valid if at least one file with matching name and size was found. */
  static bool isDirValid(const QString& path);

  /* Collect file names (up to 16). Files are opened on demand */
  bool openFiles();
  void closeFiles();

  /* Elevation in meter. If "distanceMeter" is > 0 then the center and four points around will be checked too.
   * "sampleRadiusMeter" defines a rectangle where five points are sampled and the maximum is used.*/
  float getElevation(const atools::geo::Pos& pos, float sampleRadiusMeter = 0.f);

  /* Get elevations along a great circle line. Will create a point every 500 meters and delete
   * consecutive ones with same elevation
   * "sampleRadiusMeter" defines a rectangle where five points are sampled and the maximum is used.*/
  void getElevations(geo::LineString& elevations, const atools::geo::LineString& linestring, float sampleRadiusMeter = 0.f);

private:
  friend class ::DtmTest;

  /* Source data parameters */
  static Q_DECL_CONSTEXPR int NUM_DATAFILES = 16;
  static Q_DECL_CONSTEXPR qint64 FILE_SIZE_SMALL = 103680000;
  static Q_DECL_CONSTEXPR qint64 FILE_SIZE_LARGE = 129600000;
  static Q_DECL_CONSTEXPR int TILE_COLUMNS = 10800;
  static Q_DECL_CONSTEXPR int TILE_ROWS_SMALL = 4800;
  static Q_DECL_CONSTEXPR int TILE_ROWS_LARGE = 6000;
  static Q_DECL_CONSTEXPR int GRID_COLUMNS = 4 * TILE_COLUMNS;
  static Q_DECL_CONSTEXPR int GRID_ROWS = 2 * 6000 + 2 * 4800;

  /* Distance between sampling points meter */
  static Q_DECL_CONSTEXPR float INTERPOLATION_SEGMENT_LENGTH_M = 100.f;

  /* Points are considered equal if they are equal within this range in meter */
  static Q_DECL_CONSTEXPR float SAME_ELEVATION_EPSILON_M = 1.f;

  /* Calculate file index and byte offset within file */
  qint64 calcFileOffset(int gridCol, int gridRow, int& fileIndex);
  qint64 calcFileOffset(const atools::geo::Pos& pos, int& fileIndex);
  qint64 calcFileOffset(double lonx, double laty, int& fileIndex);
  static bool fileEntryValid(const QFileInfo& fileEntry);
  void closeFile(int i);
  void openFile(int i);
  float elevationFromIndexAndOffset(int fileIndex, qint64 fileOffset);
  float elevationMax(const atools::geo::Pos& pos, float sampleRadiusMeter);

  QString dataDir;
  QVector<QString> dataFilenames;
  QVector<QFile *> dataFiles;
  QVector<QDataStream *> dataStreams;
};

} // namespace common
} // namespace fs
} // namespace atools

#endif // ATOOLS_DTM_GLOBEREADER_H
