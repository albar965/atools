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

#ifndef BGLREADERPROGRESSINFO_H
#define BGLREADERPROGRESSINFO_H

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

  const QString& getBglFilepath() const
  {
    return bglFilepath;
  }

  const QString& getOtherAction() const
  {
    return otherAction;
  }

  int getTotal() const
  {
    return total;
  }

  bool getNewFile() const
  {
    return newFile;
  }

  bool getNewSceneryArea() const
  {
    return newSceneryArea;
  }

  bool getNewOther() const
  {
    return newOther;
  }

private:
  friend atools::fs::db::ProgressHandler;
  int total = 0, current = 0;
  bool newFile = false, newSceneryArea = false, newOther = false;
  QString bglFilepath, otherAction;
  const atools::fs::scenery::SceneryArea *sceneryArea = nullptr;

};

} // namespace fs
} // namespace atools

#endif // BGLREADERPROGRESSINFO_H
