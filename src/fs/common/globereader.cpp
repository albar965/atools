/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "fs/common/globereader.h"
#include "exception.h"
#include "atools.h"

#include "geo/pos.h"
#include "geo/linestring.h"
#include "geo/line.h"

#include <cmath>
#include <QDataStream>
#include <QDir>
#include <QHash>

using atools::geo::Pos;
using atools::geo::Line;
using atools::geo::LineString;

namespace atools {
namespace fs {
namespace common {

/* Distance between sampling points meter */
const static float INTERPOLATION_SEGMENT_LENGTH_M = 400.f;
const static float INTERPOLATION_SEGMENT_LENGTH_PRECISION_M = 200.f;

/* Points are considered equal if they are equal within this range in meter */
const static float SAME_ELEVATION_EPSILON_M = 1.f;

/* Calculate file index and byte offset within file */

// inline since it is used only once
inline int calcFileOffsetFromColRow(int gridCol, int gridRow, int& fileIndex)
{
  // Normalize / rollover values
  while(gridCol >= GlobeReader::GRID_COLUMNS)
    gridCol -= GlobeReader::GRID_COLUMNS;

  while(gridCol < 0)
    gridCol += GlobeReader::GRID_COLUMNS;

  while(gridRow >= GlobeReader::GRID_ROWS)
    gridRow -= GlobeReader::GRID_ROWS;

  while(gridRow < 0)
    gridRow += GlobeReader::GRID_ROWS;

  // Column in file/tile grid
  int fileCol = gridCol / GlobeReader::TILE_COLUMNS;
  // Column in file
  int fileColOffset = gridCol % GlobeReader::TILE_COLUMNS;

  // Row in file/tile grid and row in file
  int fileRow, fileRowOffset;
  if(gridRow < GlobeReader::TILE_ROWS_SMALL)
  {
    fileRow = 0;
    fileRowOffset = gridRow;
  }
  else if(gridRow < GlobeReader::TILE_ROWS_SMALL + GlobeReader::TILE_ROWS_LARGE)
  {
    fileRow = 1;
    fileRowOffset = gridRow - GlobeReader::TILE_ROWS_SMALL;
  }
  else if(gridRow < GlobeReader::TILE_ROWS_SMALL + GlobeReader::TILE_ROWS_LARGE * 2)
  {
    fileRow = 2;
    fileRowOffset = gridRow - (GlobeReader::TILE_ROWS_SMALL + GlobeReader::TILE_ROWS_LARGE);
  }
  else if(gridRow < GlobeReader::TILE_ROWS_SMALL * 2 + GlobeReader::TILE_ROWS_LARGE * 2)
  {
    fileRow = 3;
    fileRowOffset = gridRow - (GlobeReader::TILE_ROWS_SMALL + GlobeReader::TILE_ROWS_LARGE * 2);
  }
  else
    throw atools::Exception(QStringLiteral("Invalid grid row %1").arg(gridRow));

  // Index in file array
  fileIndex = fileRow * 4 + fileCol;

  // Word offset in file
  int offset = fileColOffset + fileRowOffset * GlobeReader::TILE_COLUMNS;

  // Byte offset in file
  return offset * 2;
}

inline int calcFileOffset(double lonx, double laty, int& fileIndex)
{
  return calcFileOffsetFromColRow(static_cast<int>(GlobeReader::GRID_COLUMNS * (lonx + 180.) / 360.),
                                  static_cast<int>(GlobeReader::GRID_ROWS * (180. - (static_cast<double>(laty) + 90.)) / 180.), fileIndex);
}

// Test method
int GlobeReader::calcFileOffsetTest(double lonx, double laty, int& fileIndex)
{
  return calcFileOffsetFromColRow(static_cast<int>(GlobeReader::GRID_COLUMNS * (lonx + 180.) / 360.),
                                  static_cast<int>(GlobeReader::GRID_ROWS * (180. - (static_cast<double>(laty) + 90.)) / 180.), fileIndex);
}

// =========================================================
GlobeReader::GlobeReader(const QString& dataDirParam)
  : fileCache(CACHE_MAX_BYTES_DEFAULT), dataDir(dataDirParam)
{
  // Fill lists with empty values
  dataFiles.fill(nullptr, NUM_DATAFILES);
  dataFilenames.fill(QStringLiteral(), NUM_DATAFILES);
}

GlobeReader::~GlobeReader()
{
  closeFiles();
}

bool GlobeReader::isDirValid(const QString& path)
{
  QDir dir(path);
  const QFileInfoList entries = dir.entryInfoList({QStringLiteral("???g")}, QDir::Files, QDir::Name | QDir::IgnoreCase);
  for(const QFileInfo& file : entries)
  {
    if(fileEntryValid(file))
      return true;
  }

  return false;
}

bool GlobeReader::fileEntryValid(const QFileInfo& fileEntry)
{
  return fileEntry.exists() && fileEntry.isReadable() && fileEntry.isFile() &&
         QDir::match(QStringLiteral("[a-p][1-9][0-9][gb]"), fileEntry.fileName()) &&
         (fileEntry.size() == FILE_SIZE_LARGE || fileEntry.size() == FILE_SIZE_SMALL);
}

bool GlobeReader::openFiles()
{
  closeFiles();
  valid = false;

  if(!isDirValid(dataDir))
    return false;

  QDir dir(dataDir);

  // Read only filenames here
  for(const QFileInfo& fileEntry : dir.entryInfoList({QStringLiteral("????")}, QDir::Files, QDir::Name | QDir::IgnoreCase))
  {
    if(fileEntryValid(fileEntry))
    {
      // Revised files will be overwritten
      int index = static_cast<int>(fileEntry.fileName().at(0).toLatin1() - 'a');
      dataFilenames[index] = fileEntry.filePath();
      valid = true;
    }
    else
      qWarning() << "Found invalid file" << fileEntry.filePath();
  }
  return true;
}

void GlobeReader::openFile(int i)
{
  if(atools::inRange(dataFilenames, i))
  {
    const QString& name = dataFilenames.at(i);
    if(!name.isEmpty() && dataFiles[i] == nullptr)
    {
      qDebug() << Q_FUNC_INFO << name;
      dataFiles[i] = new QFile(name);
      if(!dataFiles[i]->open(QIODevice::ReadOnly))
      {
        closeFile(i);

        // Clear filename to avoid reopening
        dataFilenames[i].clear();
        qWarning() << "Cannot open file" << name;
      }
    }
  }
  else
    qWarning() << Q_FUNC_INFO << "Filename index" << i << "out of range";
}

void GlobeReader::closeFile(int i)
{
  if(dataFiles[i] != nullptr)
  {
    dataFiles[i]->close();
    delete dataFiles[i];
    dataFiles[i] = nullptr;
  }
}

void GlobeReader::closeFiles()
{
  for(int i = 0; i < NUM_DATAFILES; i++)
    closeFile(i);
}

float GlobeReader::getElevation(const atools::geo::Pos& pos, float sampleRadiusMeter)
{
  if(!valid || !pos.isValid())
    return ELEVATION_INVALID;

  // Used different numbers of probe rectangles depeding on sample radius
  const static QVarLengthArray<float, 5> FACTOR_POINT({0.f}); // Get value at position
  const static QVarLengthArray<float, 5> FACTOR_ONE_RECT({1.f}); // Get maximum around pos using one rectangles
  const static QVarLengthArray<float, 5> FACTOR_TWO_RECT({1.f, 0.5f}); // Get maximum around pos using two rectangles
  const static QVarLengthArray<float, 5> FACTOR_THREE_RECT({1.f, 0.75f, 0.5f, 0.25f}); // Get maximum around pos using four rectangles

  const QVarLengthArray<float, 5> *factor = &FACTOR_POINT;
  if(sampleRadiusMeter < 0.001f)
    factor = &FACTOR_POINT;
  else if(sampleRadiusMeter < 0.5f)
    factor = &FACTOR_ONE_RECT;
  else if(sampleRadiusMeter < atools::geo::nmToMeter(4.f))
    factor = &FACTOR_TWO_RECT;
  else
    factor = &FACTOR_THREE_RECT;

  // Calculate maximum from up to five samples
  float maxAlt = ELEVATION_OCEAN;
  for(float f : *factor)
  {
    float elevation = elevationMax(pos, f * sampleRadiusMeter);
    if(elevation < ELEVATION_INVALID)
      maxAlt = std::max(elevation, maxAlt);
  }

  return maxAlt;
}

float GlobeReader::elevationMax(const geo::Pos& pos, float sampleRadiusMeter)
{
  int fileIndex, fileOffset;
  if(sampleRadiusMeter < 0.01f)
  {
    // Get at exact position ====================
    fileOffset = calcFileOffset(pos.getLonX(), pos.getLatY(), fileIndex);
    return elevationFromIndexAndOffset(fileIndex, fileOffset);
  }
  else
  {
    // Get from rectangle ====================
    QVarLengthArray<Pos, 10> positions;

    // Center position
    positions.append(pos);

    // Build a rectangle around the position ==================
    atools::geo::Rect rect(pos, sampleRadiusMeter, true /* fast */);

    for(const atools::geo::Rect& splitRect : rect.splitAtAntiMeridian())
    {
      positions.append(splitRect.getTopLeft());
      positions.append(splitRect.getTopRight());
      positions.append(splitRect.getBottomRight());
      positions.append(splitRect.getBottomLeft());

      if(sampleRadiusMeter > atools::geo::nmToMeter(1.))
      {
        // Also use center for bigger rectangles
        positions.append(splitRect.getTopCenter());
        positions.append(splitRect.getRightCenter());
        positions.append(splitRect.getBottomCenter());
        positions.append(splitRect.getLeftCenter());
      }
    }

    float maxAlt = ELEVATION_OCEAN;
    for(const Pos& pos : positions)
    {
      // Get maximum for all positions
      fileOffset = calcFileOffset(pos.getLonX(), pos.getLatY(), fileIndex);
      float elevation = elevationFromIndexAndOffset(fileIndex, fileOffset);
      if(elevation < ELEVATION_INVALID)
        maxAlt = std::max(elevation, maxAlt);
    }

    return maxAlt;
  }
}

void GlobeReader::getElevations(atools::geo::LineString& elevations, const atools::geo::LineString& linestring, float sampleRadiusMeter,
                                bool precision)
{
  if(linestring.isEmpty() || !valid)
    return;

  if(linestring.size() == 1)
    elevations.append(linestring.constFirst().alt(getElevation(linestring.constFirst(), sampleRadiusMeter)));
  else
  {
    LineString positions;
    for(int i = 0; i < linestring.size() - 1; i++)
    {
      const Line line = Line(linestring.at(i), linestring.at(i + 1));

      float length = line.lengthMeter();

      positions.clear();
      int numPoints = static_cast<int>(length / (precision ? INTERPOLATION_SEGMENT_LENGTH_PRECISION_M : INTERPOLATION_SEGMENT_LENGTH_M));
      line.interpolatePoints(length, numPoints, positions);

      Pos lastDropped;
      for(const Pos& pos : std::as_const(positions))
      {
        float elevation = getElevation(pos, sampleRadiusMeter);

        if(!elevations.isEmpty())
        {
          if(atools::almostEqual(elevations.constLast().getAltitude(), elevation, SAME_ELEVATION_EPSILON_M))
          {
            // Drop points with similar altitude
            lastDropped = pos;
            lastDropped.setAltitude(elevation);
            continue;
          }
          else if(lastDropped.isValid())
          {
            // Add last point of a stretch with similar altitude
            elevations.append(lastDropped);
            lastDropped = Pos();
          }
        }

        elevations.append(pos.alt(elevation));
      }
    }

    elevations.append(linestring.constLast());
    if(!elevations.isEmpty())
      elevations.last().setAltitude(getElevation(elevations.constLast(), sampleRadiusMeter));
  }
}

void GlobeReader::setCacheMaxBytes(qsizetype maxBytes)
{
  fileCache.setMaxCost(std::min(maxBytes, FILE_SIZE_LARGE * 2L));
}

void GlobeReader::clearCache()
{
  fileCache.clear();
}

inline float fileBytesToElevation(QByteArray *fileBytes, int fileOffset)
{
  if(fileBytes != nullptr && !fileBytes->isEmpty())
  {
    // LittleEndian
    const char *data = fileBytes->constData();
    unsigned short e0 = (static_cast<unsigned short>(data[fileOffset])) & 0xff;
    unsigned short e1 = (static_cast<unsigned short>(data[fileOffset + 1]) << 8) & 0xff00;

    return static_cast<float>(static_cast<qint16>(e0 | e1));
  }
  else
    return ELEVATION_INVALID;
}

float GlobeReader::elevationFromIndexAndOffset(int fileIndex, int fileOffset)
{
  float elevation = ELEVATION_INVALID;
  QByteArray *fileBytes = fileCache.object(fileIndex);
  if(fileBytes != nullptr)
    // File found in cache
    elevation = fileBytesToElevation(fileBytes, fileOffset);
  else
  {
    // File not in cache
    openFile(fileIndex);
    QFile *dataFile = dataFiles[fileIndex];

    if(dataFile != nullptr)
    {
      fileBytes = new QByteArray;
      *fileBytes = dataFile->readAll();
      if(!dataFile->atEnd() || dataFile->error() != QFileDevice::NoError)
      {
        // Add empty entry to indicate error reading file
        fileCache.insert(fileIndex, new QByteArray());
        qWarning() << Q_FUNC_INFO << "Error reading" << dataFile->fileName() << dataFile->errorString();
      }
      else
      {
        // Insert into cache and return elevation - cost is bytes
        fileCache.insert(fileIndex, fileBytes, fileBytes->size());
        elevation = fileBytesToElevation(fileBytes, fileOffset);
      }
    }
    else
      // Add empty entry to indicate missing file
      fileCache.insert(fileIndex, new QByteArray());

#ifdef DEBUG_INFORMATION
    qDebug() << Q_FUNC_INFO << "totalCost" << fileCache.totalCost() << "maxCost" << fileCache.maxCost();
#endif
  }
  return elevation;
}

} // namespace common
} // namespace fs
} // namespace atools
