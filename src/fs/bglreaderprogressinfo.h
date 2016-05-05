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

#ifndef ATOOLS_FS_BGLREADERPROGRESSINFO_H
#define ATOOLS_FS_BGLREADERPROGRESSINFO_H

#include <QString>

namespace atools {
namespace fs {

namespace scenery {
class SceneryArea;
}
namespace db {
class ProgressHandler;
}
class BglReaderProgressInfo
{
public:
  BglReaderProgressInfo();

  const QString& getSceneryTitle() const;
  const QString& getSceneryPath() const;

  int getCurrent() const
  {
    return current;
  }

  const QString& getBglFilename() const
  {
    return bglFilename;
  }

  const QString& getOtherAction() const
  {
    return otherAction;
  }

  int getTotal() const
  {
    return total;
  }

  bool isNewFile() const
  {
    return newFile;
  }

  bool isNewSceneryArea() const
  {
    return newSceneryArea;
  }

  bool isNewOther() const
  {
    return newOther;
  }

  bool isFirstCall() const
  {
    return firstCall;
  }

  bool isLastCall() const
  {
    return lastCall;
  }

  int getNumFiles() const
  {
    return numFiles;
  }

  int getNumAirports() const
  {
    return numAirports;
  }

  int getNumNamelists() const
  {
    return numNamelists;
  }

  int getNumVors() const
  {
    return numVors;
  }

  int getNumIls() const
  {
    return numIls;
  }

  int getNumNdbs() const
  {
    return numNdbs;
  }

  int getNumMarker() const
  {
    return numMarker;
  }

  int getNumBoundaries() const
  {
    return numBoundaries;
  }

  int getNumWaypoints() const
  {
    return numWaypoints;
  }

  int getNumObjectsWritten() const
  {
    return numObjectsWritten;
  }

private:
  friend atools::fs::db::ProgressHandler;

  int numFiles = 0, numAirports = 0, numNamelists = 0, numVors = 0, numIls = 0, numNdbs = 0, numMarker = 0,
      numBoundaries = 0, numWaypoints = 0, numObjectsWritten = 0;

  int total = 0, current = 0;
  bool newFile = false, newSceneryArea = false, newOther = false, firstCall = true, lastCall = false;
  QString bglFilename, otherAction;
  const atools::fs::scenery::SceneryArea *sceneryArea = nullptr;

};

} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_BGLREADERPROGRESSINFO_H
