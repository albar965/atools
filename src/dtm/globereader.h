/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

namespace dtm {

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

  /* Elevation in meter */
  float getElevation(const atools::geo::Pos& pos);

  /* Get elevations along a great circle line. Will create a point every 500 meters and delete
   * consecutive ones with same elevation */
  void getElevations(geo::LineString& elevations, const atools::geo::LineString& linestring);

private:
  friend class::DtmTest;

  /* Source data parameters */
  static Q_DECL_CONSTEXPR int NUM_DATAFILES = 16;
  static Q_DECL_CONSTEXPR qint64 FILE_SIZE_SMALL = 103680000;
  static Q_DECL_CONSTEXPR qint64 FILE_SIZE_LARGE = 129600000;
  static Q_DECL_CONSTEXPR int TILE_COLUMNS = 10800;
  static Q_DECL_CONSTEXPR int TILE_ROWS_SMALL = 4800;
  static Q_DECL_CONSTEXPR int TILE_ROWS_LARGE = 6000;
  static Q_DECL_CONSTEXPR int GRID_COLUMNS = 4 * TILE_COLUMNS;
  static Q_DECL_CONSTEXPR int GRID_ROWS = 2 * 6000 + 2 * 4800;

  /* Cut segments into points if it is shorter than this value in meter */
  static Q_DECL_CONSTEXPR float MIN_LENGTH_FOR_INTERPOLATION = 1000.f;
  /* Distance between sampling points meter */
  static Q_DECL_CONSTEXPR float INTERPOLATION_SEGMENT_LENGTH = 250.f;
  /* Points are considered equal if they are equal within this range in meter */
  static Q_DECL_CONSTEXPR float SAME_ELEVATION_EPSILON = 1.f;

  /* Calculate file index and byte offset within file */
  qint64 calcFileOffset(int gridCol, int gridRow, int& fileIndex);
  qint64 calcFileOffset(const atools::geo::Pos& pos, int& fileIndex);
  qint64 calcFileOffset(double lonx, double laty, int& fileIndex);
  static bool fileEntryValid(const QFileInfo& fileEntry);
  void closeFile(int i);
  void openFile(int i);

  QString dataDir;
  QVector<QString> dataFilenames;
  QVector<QFile *> dataFiles;
  QVector<QDataStream *> dataStreams;
};

} // namespace dtm
} // namespace atools

#endif // ATOOLS_DTM_GLOBEREADER_H
