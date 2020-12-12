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

#include "fs/progresshandler.h"
#include "fs/navdatabaseoptions.h"
#include "fs/scenery/sceneryarea.h"

#include <QDebug>

namespace atools {
namespace fs {

ProgressHandler::ProgressHandler(const NavDatabaseOptions *options)
{
  if(options->getProgressCallback() != nullptr)
    handler = options->getProgressCallback();
}

void ProgressHandler::increaseCurrent(int increase)
{
  info.current += increase;
}

bool ProgressHandler::reportOtherMsg(const QString& otherAction)
{
  info.otherAction = otherAction;
  info.newFile = false;
  info.newSceneryArea = false;
  info.newOther = true;

  return callHandler();
}

bool ProgressHandler::reportOther(const QString& otherAction, int current, bool silent)
{
  info.lastCurrent = info.current;
  if(current != -1)
    info.current = current;
  else
    info.current++;
  info.otherAction = otherAction;

  info.newFile = false;
  info.newSceneryArea = false;
  info.newOther = true;

  if(silent)
    return false;
  else
    return callHandler();
}

bool ProgressHandler::reportOtherInc(const QString& otherAction, int increment)
{
  info.lastCurrent = info.current;
  info.current += increment;
  info.otherAction = otherAction;

  info.newFile = false;
  info.newSceneryArea = false;
  info.newOther = true;

  return callHandler();
}

void ProgressHandler::reportError()
{
  info.numErrors++;
}

void ProgressHandler::reportErrors(int num)
{
  info.numErrors += num;
}

bool ProgressHandler::reportBglFile(const QString& bglFilepath)
{
  info.lastCurrent = info.current;
  info.current++;
  info.bglFilepath = bglFilepath;

  info.newFile = true;
  info.newSceneryArea = false;
  info.newOther = false;

  return callHandler();
}

bool ProgressHandler::reportFinish()
{
  info.lastCall = true;
  info.newFile = false;
  info.newSceneryArea = false;
  info.newOther = false;

  qDebug() << Q_FUNC_INFO << "info.current" << info.current;

  return callHandler();
}

void ProgressHandler::setTotal(int total)
{
  info.total = total;
}

void ProgressHandler::reset()
{
  info.numErrors = 0;
  info.current = 0;
  info.lastCurrent = 0;
  info.sceneryArea = nullptr;
  info.bglFilepath.clear();
  info.newFile = false;
  info.newSceneryArea = false;
  info.newOther = false;
  info.firstCall = true;
  info.lastCall = false;
}

bool ProgressHandler::reportSceneryArea(const scenery::SceneryArea *sceneryArea)
{
  info.lastCurrent = info.current;
  info.current++;
  info.sceneryArea = sceneryArea;

  info.newFile = false;
  info.newSceneryArea = true;
  info.newOther = false;

  return callHandler();
}

bool ProgressHandler::callHandler()
{
  bool retval = false;

  // Alway call default handler - this one cannot call cancel
  defaultHandler(info);

  if(handler != nullptr)
    // Call user handler
    retval = handler(info);

  if(info.firstCall)
    info.firstCall = false;

  return retval;
}

/*
 * Default handler prints to console or log only
 */
void ProgressHandler::defaultHandler(const atools::fs::NavDatabaseProgress& inf)
{
  // Using "=P===" for easier recognition in the log file

  if(inf.isNewFile())
  {
    qInfo() << "=P===" << numbersAsString(inf) << inf.getBglFileName();
  }

  if(inf.isNewSceneryArea())
  {
    qInfo() << "=P=====================================================================";
    qInfo() << "=P===" << numbersAsString(inf) << inf.getSceneryTitle();
    qInfo() << "=P===" << inf.getSceneryPath();
  }

  if(inf.isNewOther())
  {
    qInfo() << "=P===" << numbersAsString(inf) << inf.getOtherAction();
  }
}

QString ProgressHandler::numbersAsString(const atools::fs::NavDatabaseProgress& inf)
{
  return QString("%1 of %2 (%3 %) [%4]").
         arg(std::min(inf.current, inf.total)).
         arg(inf.total).
         arg(inf.total > 0 ? std::min(100 * inf.current / inf.total, 100) : 0).
         arg(inf.current - inf.lastCurrent);
}

} // namespace fs
} // namespace atools
