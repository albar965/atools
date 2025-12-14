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

#ifndef ATOOLS_PROGRESSHANDLER_H
#define ATOOLS_PROGRESSHANDLER_H

#include "fs/navdatabaseprogress.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace scenery {
class SceneryArea;
}

/*
 * Progress handler. Fills the NavDatabaseProgress object with information and calls the progress callback.
 */
class ProgressHandler
{
public:
  void setProgressCallback(const atools::fs::NavDatabaseOptions::ProgressCallbackType& value)
  {
    progressCallback = value;
  }

  void setCallDefaultCallback(bool value)
  {
    callDefaultCallback = value;
  }

  /*
   * Increment progress by one and send message about new scenery area
   */
  bool reportSceneryArea(const atools::fs::scenery::SceneryArea *sceneryArea);

  /*
   * Increment progress by one and send message about new BGL file
   */
  bool reportBglFile(const QString& bglFilepath);

  /*
   * Increase number of errors for BGL reading exceptions or scenery errors (directory not found or others).
   */
  void reportError();
  void reportErrors(int num);

  /*
   * Send the last report
   */
  bool reportFinish();

  /*
   * Increment progress by one and send message about other processes like script execution
   */
  bool reportOther(const QString& otherAction, int current = -1, bool silent = false);

  /*
   * Increment progress by increment and send message about other processes like script execution
   */
  bool reportOtherInc(const QString& otherAction, int increment);

  /* Only send message without incrementing progress */
  bool reportOtherMsg(const QString& otherAction);

  /* set total amount of progress steps */
  void setTotal(int total);

  /* Reset all */
  void reset();

  /* Increase current progress counter without sending a message */
  void increaseCurrent(int increase);

  /* Set current number of BGL files */
  void setNumFiles(int value)
  {
    info.numFiles = value;
  }

  void setNumAirports(int value)
  {
    info.numAirports = value;
  }

  void setNumNamelists(int value)
  {
    info.numNamelists = value;
  }

  void setNumVors(int value)
  {
    info.numVors = value;
  }

  void setNumIls(int value)
  {
    info.numIls = value;
  }

  void setNumNdbs(int value)
  {
    info.numNdbs = value;
  }

  void setNumMarker(int value)
  {
    info.numMarker = value;
  }

  void setNumBoundaries(int value)
  {
    info.numBoundaries = value;
  }

  void setNumWaypoints(int value)
  {
    info.numWaypoints = value;
  }

  void setNumObjectsWritten(int value)
  {
    info.numObjectsWritten = value;
  }

  void incNumFiles(int value = 1)
  {
    info.numFiles += value;
  }

  void incNumAirports(int value = 1)
  {
    info.numAirports += value;
  }

  void incNumNamelists(int value = 1)
  {
    info.numNamelists += value;
  }

  void incNumVors(int value = 1)
  {
    info.numVors += value;
  }

  void incNumIls(int value = 1)
  {
    info.numIls += value;
  }

  void incNumNdbs(int value = 1)
  {
    info.numNdbs += value;
  }

  void incNumMarker(int value = 1)
  {
    info.numMarker += value;
  }

  void incNumBoundaries(int value = 1)
  {
    info.numBoundaries += value;
  }

  void incNumWaypoints(int value = 1)
  {
    info.numWaypoints += value;
  }

  void incNumObjectsWritten(int value = 1)
  {
    info.numObjectsWritten += value;
  }

private:
  bool callDefaultCallback = true;
  void defaultProgressCallback();

  atools::fs::NavDatabaseOptions::ProgressCallbackType progressCallback;

  atools::fs::NavDatabaseProgress info;

  bool callHandler();

  static QString numbersAsString(const atools::fs::NavDatabaseProgress& inf);

};

} // namespace fs
} // namespace atools

#endif // ATOOLS_PROGRESSHANDLER_H
