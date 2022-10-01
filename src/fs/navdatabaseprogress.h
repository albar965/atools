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

#ifndef ATOOLS_FS_NAVDATABASEPROGRESS_H
#define ATOOLS_FS_NAVDATABASEPROGRESS_H

#include <QString>

namespace atools {
namespace fs {

namespace scenery {
class SceneryArea;
}
class ProgressHandler;

/*
 * Progress information that is passed to the progress callback.
 */
class NavDatabaseProgress
{
public:
  NavDatabaseProgress();

  /*
   * @return Currently processed scenery area title
   */
  const QString& getSceneryTitle() const;

  /*
   * @return Currently processed scenery area path
   */
  const QString& getSceneryPath() const;

  /*
   * @return current progress step
   */
  int getCurrent() const
  {
    return current;
  }

  /* Progress step before incrementing step */
  int getLastCurrent() const
  {
    return lastCurrent;
  }

  /*
   * @return Currently processed BGL filename
   */
  QString getFileName() const;

  /*
   * @return Currently processed BGL filepath
   */
  const QString& getFilePath() const
  {
    return filepath;
  }

  /*
   * @return currently processed action that not a BGL file
   */
  const QString& getOtherAction() const
  {
    return otherAction;
  }

  /*
   * @return total progress steps
   */
  int getTotal() const
  {
    return total;
  }

  /*
   * @return true if processing started a new BGL file
   */
  bool isNewFile() const
  {
    return newFile;
  }

  /*
   * @return true if processing started a new scenery area
   */
  bool isNewSceneryArea() const
  {
    return newSceneryArea;
  }

  /*
   * @return true if processing started a new step
   */
  bool isNewOther() const
  {
    return newOther;
  }

  /*
   * @return true if this is the first progress report
   */
  bool isFirstCall() const
  {
    return firstCall;
  }

  /*
   * @return true if this is the last progress report
   */
  bool isLastCall() const
  {
    return lastCall;
  }

  /*
   * @return number of files processed so far
   */
  int getNumFiles() const
  {
    return numFiles;
  }

  /*
   * @return number of airports processed so far
   */
  int getNumAirports() const
  {
    return numAirports;
  }

  /*
   * @return number of name lists processed so far
   */
  int getNumNamelists() const
  {
    return numNamelists;
  }

  /*
   * @return number of VORs processed so far
   */
  int getNumVors() const
  {
    return numVors;
  }

  /*
   * @return number of ILS processed so far
   */
  int getNumIls() const
  {
    return numIls;
  }

  /*
   * @return number of NDBs processed so far
   */
  int getNumNdbs() const
  {
    return numNdbs;
  }

  /*
   * @return number of markers processed so far
   */
  int getNumMarker() const
  {
    return numMarker;
  }

  /*
   * @return number of airspace boundaries processed so far
   */
  int getNumBoundaries() const
  {
    return numBoundaries;
  }

  /*
   * @return number of waypoints processed so far
   */
  int getNumWaypoints() const
  {
    return numWaypoints;
  }

  /*
   * @return total number of database objects written processed so far
   */
  int getNumObjectsWritten() const
  {
    return numObjectsWritten;
  }

  /*
   * @return total number of errors/exceptions during BGL loading
   */
  int getNumErrors() const
  {
    return numErrors;
  }

private:
  friend atools::fs::ProgressHandler;

  int numFiles = 0, numAirports = 0, numNamelists = 0, numVors = 0, numIls = 0, numNdbs = 0, numMarker = 0,
      numBoundaries = 0, numWaypoints = 0, numObjectsWritten = 0, numErrors = 0;

  int total = 0, current = 0, lastCurrent = 0;
  bool newFile = false, newSceneryArea = false, newOther = false, firstCall = true, lastCall = false;
  QString filepath, otherAction;
  const atools::fs::scenery::SceneryArea *sceneryArea = nullptr;

};

} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_NAVDATABASEPROGRESS_H
