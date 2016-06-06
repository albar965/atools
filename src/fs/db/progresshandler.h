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

#ifndef PROGRESSHANDLER_H
#define PROGRESSHANDLER_H

#include "fs/bglreaderprogressinfo.h"
#include "fs/bglreaderoptions.h"

namespace atools {
namespace fs {
namespace scenery {
class SceneryArea;
}
namespace db {

class ProgressHandler
{
public:
  ProgressHandler(const atools::fs::BglReaderOptions *options);

  bool report(const atools::fs::scenery::SceneryArea *sceneryArea, int current = -1);
  bool report(const QString& bglFilepath, int current = -1);
  bool reportFinish();
  bool reportOther(const QString& otherAction, int current = -1);

  /* Only send message without incrementing progress */
  bool reportOtherMsg(const QString& otherAction);

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

  void setTotal(int total);

  void reset();

  void increaseCurrent(int increase);

  void setCurrent(int value);

private:
  void defaultHandler(const atools::fs::BglReaderProgressInfo& inf);

  atools::fs::BglReaderOptions::ProgressCallbackType handler = nullptr;

  atools::fs::BglReaderProgressInfo info;

  bool callHandler();

  QString numbersAsString(const atools::fs::BglReaderProgressInfo& inf);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // PROGRESSHANDLER_H
