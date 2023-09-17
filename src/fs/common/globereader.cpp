/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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

GlobeReader::GlobeReader(const QString& dataDirParam)
  : dataDir(dataDirParam)
{
  dataFiles.fill(nullptr, NUM_DATAFILES);
  dataStreams.fill(nullptr, NUM_DATAFILES);
  dataFilenames.fill(QString(), NUM_DATAFILES);
}

GlobeReader::~GlobeReader()
{
  closeFiles();
}

bool GlobeReader::isDirValid(const QString& path)
{
  QDir dir(path);
  const QFileInfoList entries = dir.entryInfoList({"???g"}, QDir::Files, QDir::Name | QDir::IgnoreCase);
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
         QDir::match("[a-p][1-9][0-9][gb]", fileEntry.fileName()) &&
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
  const QFileInfoList entries = dir.entryInfoList({"????"}, QDir::Files, QDir::Name | QDir::IgnoreCase);
  for(const QFileInfo& fileEntry : entries)
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
  const QString& name = dataFilenames.at(i);
  if(!name.isEmpty() && dataFiles[i] == nullptr)
  {
    qDebug() << Q_FUNC_INFO << name;
    dataFiles[i] = new QFile(name);
    if(dataFiles[i]->open(QIODevice::ReadOnly))
    {
      dataStreams[i] = new QDataStream(dataFiles[i]);
      dataStreams[i]->setByteOrder(QDataStream::LittleEndian);
    }
    else
    {
      closeFile(i);
      // Clear filename to avoid reopening
      dataFilenames[i].clear();
      qWarning() << "Cannot open file" << name;
    }
  }
}

void GlobeReader::closeFile(int i)
{
  if(dataStreams[i] != nullptr)
  {
    delete dataStreams[i];
    dataStreams[i] = nullptr;
  }

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
    return atools::fs::common::INVALID;

  if(sampleRadiusMeter > 0.f)
    // Get maximum around pos
    return elevationMax(pos, sampleRadiusMeter);
  else
  {
    int fileIndex;
    qint64 fileOffset = calcFileOffset(pos.getLonX(), pos.getLatY(), fileIndex);
    return elevationFromIndexAndOffset(fileIndex, fileOffset);
  }
}

float GlobeReader::elevationMax(const geo::Pos& pos, float sampleRadiusMeter)
{
  // Collect file indexes and offsets - use set to remove duplicates
  QSet<std::pair<int, qint64> > indexes;
  // Center point
  int fileIndex;
  qint64 fileOffset = calcFileOffset(pos.getLonX(), pos.getLatY(), fileIndex);
  indexes.insert(std::make_pair(fileIndex, fileOffset));

  // Build a rectangle around the position
  atools::geo::Rect rect(pos, sampleRadiusMeter, true /* fast */);

  // Get split if crossing anti-meridian
  for(const atools::geo::Rect& r : rect.splitAtAntiMeridian())
  {
    // Top left
    fileOffset = calcFileOffset(r.getTopLeft().getLonX(), r.getTopLeft().getLatY(), fileIndex);
    indexes.insert(std::make_pair(fileIndex, fileOffset));

    // Top right
    fileOffset = calcFileOffset(r.getTopRight().getLonX(), r.getTopRight().getLatY(), fileIndex);
    indexes.insert(std::make_pair(fileIndex, fileOffset));

    // Bottom right
    fileOffset = calcFileOffset(r.getBottomRight().getLonX(), r.getBottomRight().getLatY(), fileIndex);
    indexes.insert(std::make_pair(fileIndex, fileOffset));

    // Bottom left
    fileOffset = calcFileOffset(r.getBottomLeft().getLonX(), r.getBottomLeft().getLatY(), fileIndex);
    indexes.insert(std::make_pair(fileIndex, fileOffset));
  }

  // Calculate maximum from up to five samples
  float maxAlt = 0.f;
  for(const std::pair<int, qint64>& index : indexes)
  {
    float elevation = elevationFromIndexAndOffset(index.first, index.second);
    if(elevation > atools::fs::common::OCEAN && elevation < atools::fs::common::INVALID)
      maxAlt = std::max(elevation, maxAlt);
  }
  return maxAlt;
}

float GlobeReader::elevationFromIndexAndOffset(int fileIndex, qint64 fileOffset)
{
  openFile(fileIndex);
  QFile *dataFile = dataFiles[fileIndex];

  if(dataFile != nullptr)
  {
    dataFile->seek(fileOffset);
    QDataStream *dataStream = dataStreams[fileIndex];

    qint16 elevation;
    *dataStream >> elevation;
    return elevation;
  }
  else
    return INVALID;
}

void GlobeReader::getElevations(atools::geo::LineString& elevations, const atools::geo::LineString& linestring, float sampleRadiusMeter)
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
      Line line = Line(linestring.at(i), linestring.at(i + 1));

      float length = line.lengthMeter();

      positions.clear();
      line.interpolatePoints(length, static_cast<int>(length / INTERPOLATION_SEGMENT_LENGTH_M), positions);

      Pos lastDropped;
      for(const Pos& pos : qAsConst(positions))
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

qint64 GlobeReader::calcFileOffset(const atools::geo::Pos& pos, int& fileIndex)
{
  return calcFileOffset(pos.getLonX(), pos.getLatY(), fileIndex);
}

qint64 GlobeReader::calcFileOffset(double lonx, double laty, int& fileIndex)
{
  int gridCol = static_cast<int>(GRID_COLUMNS * (lonx + 180.) / 360.);
  int gridRow = static_cast<int>(GRID_ROWS * (180. - (static_cast<double>(laty) + 90.)) / 180.);

  return calcFileOffset(gridCol, gridRow, fileIndex);
}

qint64 GlobeReader::calcFileOffset(int gridCol, int gridRow, int& fileIndex)
{
  // Normalize / rollover values
  while(gridCol >= GRID_COLUMNS)
    gridCol -= GRID_COLUMNS;
  while(gridCol < 0)
    gridCol += GRID_COLUMNS;

  while(gridRow >= GRID_ROWS)
    gridRow -= GRID_ROWS;
  while(gridRow < 0)
    gridRow += GRID_ROWS;

  // Column in file/tile grid
  int fileCol = gridCol / TILE_COLUMNS;
  // Column in file
  int fileColOffset = gridCol % TILE_COLUMNS;

  // Row in file/tile grid and row in file
  int fileRow, fileRowOffset;
  if(gridRow < TILE_ROWS_SMALL)
  {
    fileRow = 0;
    fileRowOffset = gridRow;
  }
  else if(gridRow < TILE_ROWS_SMALL + TILE_ROWS_LARGE)
  {
    fileRow = 1;
    fileRowOffset = gridRow - TILE_ROWS_SMALL;
  }
  else if(gridRow < TILE_ROWS_SMALL + TILE_ROWS_LARGE * 2)
  {
    fileRow = 2;
    fileRowOffset = gridRow - (TILE_ROWS_SMALL + TILE_ROWS_LARGE);
  }
  else if(gridRow < TILE_ROWS_SMALL * 2 + TILE_ROWS_LARGE * 2)
  {
    fileRow = 3;
    fileRowOffset = gridRow - (TILE_ROWS_SMALL + TILE_ROWS_LARGE * 2);
  }
  else
    throw atools::Exception(QString("Invalid grid row %1").arg(gridRow));

  // Index in file array
  fileIndex = fileRow * 4 + fileCol;

  // Word offset in file
  qint64 offset = fileColOffset + fileRowOffset * TILE_COLUMNS;

  // Byte offset in file
  return offset * 2;
}

} // namespace common
} // namespace fs
} // namespace atools
