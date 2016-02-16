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

#include "logging/loggingdefs.h"
#include "fs/db/progresshandler.h"
#include "fs/bglreaderoptions.h"
#include "fs/scenery/sceneryarea.h"

namespace atools {
namespace fs {
namespace db {

ProgressHandler::ProgressHandler(const BglReaderOptions *options)
{
  if(options->getProgressCallback() != nullptr)
    handler = options->getProgressCallback();
  else
    handler = [ = ](const atools::fs::BglReaderProgressInfo & i)->bool
    {
      return defaultHandler(i);
    };
}

bool ProgressHandler::reportProgressOther(const QString& otherAction, int current)
{
  if(current != -1)
    info.current = current;
  info.otherAction = otherAction;

  info.newFile = false;
  info.newSceneryArea = false;
  info.newOther = true;

  return call();
}

bool ProgressHandler::reportProgress(const QString& bglFilepath, int current)
{
  if(current != -1)
    info.current = current;
  info.bglFilepath = bglFilepath;

  info.newFile = true;
  info.newSceneryArea = false;
  info.newOther = false;

  return call();
}

void ProgressHandler::setTotal(int total)
{
  info.total = total;
}

bool ProgressHandler::reportProgress(const scenery::SceneryArea *sceneryArea, int current)
{
  if(current != -1)
    info.current = current;
  info.sceneryArea = sceneryArea;

  info.newFile = false;
  info.newSceneryArea = true;
  info.newOther = false;

  return call();
}

bool ProgressHandler::call()
{
  if(handler != nullptr)
    return handler(info);
  else
    return false;
}

bool ProgressHandler::defaultHandler(const atools::fs::BglReaderProgressInfo& inf)
{
  if(inf.getNewFile())
    qInfo() << "====" << inf.current << "of" << inf.total << inf.getBglFilepath();

  if(inf.getNewSceneryArea())
  {
    qInfo() << "======================================================================";
    qInfo() << "==========" << inf.current << "of" << inf.total << inf.getSceneryTitle();
    qInfo() << "==========" << inf.getSceneryPath();
  }

  if(inf.getNewOther())
    qInfo() << "====" << inf.current << "of" << inf.total << inf.getOtherAction();
  return false;
}

} // namespace writer
} // namespace fs
} // namespace atools
