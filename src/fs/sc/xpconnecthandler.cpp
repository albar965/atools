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

#include "fs/sc/xpconnecthandler.h"

#include "fs/sc/weatherrequest.h"

namespace atools {
namespace fs {
namespace sc {

// ===============================================================================================
// XpConnectHandler
// ===============================================================================================

XpConnectHandler::XpConnectHandler(DataCopyFunctionType dataCopyFunction, bool logVerbose)
  : dataCopyFunc(dataCopyFunction), verbose(logVerbose)
{

}

XpConnectHandler::~XpConnectHandler()
{
}

bool XpConnectHandler::connect()
{
  return true;
}

bool XpConnectHandler::fetchData(fs::sc::SimConnectData& data, int radiusKm, fs::sc::Options options)
{
  return dataCopyFunc(data, radiusKm, options);
}

bool XpConnectHandler::fetchWeatherData(fs::sc::SimConnectData& data)
{
  Q_UNUSED(data);
  return false;
}

void XpConnectHandler::addWeatherRequest(const fs::sc::WeatherRequest& request)
{
  Q_UNUSED(request);
}

const atools::fs::sc::WeatherRequest& XpConnectHandler::getWeatherRequest() const
{
  static atools::fs::sc::WeatherRequest dummy;
  return dummy;
}

bool XpConnectHandler::isSimRunning() const
{
  return true;
}

bool XpConnectHandler::isSimPaused() const
{
  return false;
}

atools::fs::sc::State XpConnectHandler::getState() const
{
  return STATEOK;
}

bool XpConnectHandler::isLoaded() const
{
  return true;
}

} // namespace sc
} // namespace fs
} // namespace atools
