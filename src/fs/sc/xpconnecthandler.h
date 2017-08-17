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

#ifndef ATOOLS_XPCONNECTHANDLER_H
#define ATOOLS_XPCONNECTHANDLER_H

#include "fs/sc/connecthandler.h"

#include <functional>

namespace atools {
namespace fs {
namespace sc {

typedef std::function<bool (fs::sc::SimConnectData& data, int radiusKm, fs::sc::Options options)> DataCopyFunctionType;

/*
 * Reads data from a callback function into SimConnectData.
 */
class XpConnectHandler :
  public atools::fs::sc::ConnectHandler
{
public:
  /*
   *
   * @param dataCopyFunction callback function which will get the data
   */
  XpConnectHandler(DataCopyFunctionType dataCopyFunction, bool logVerbose);
  virtual ~XpConnectHandler();

  /* Not used */
  virtual bool connect() override;

  /* Always loaded since running inside a plugin */
  virtual bool isLoaded() const override;

  /* Fetch data from the callback method into SimConnectData. Always returns true since running in plugin. */
  virtual bool fetchData(SimConnectData& data, int radiusKm, Options options) override;

  /* Not supported in X-Plane */
  virtual bool fetchWeatherData(SimConnectData& data) override;

  /* Not supported in X-Plane */
  virtual void addWeatherRequest(const WeatherRequest& request) override;

  /* Not supported in X-Plane */
  virtual const WeatherRequest& getWeatherRequest() const override;

  /* Always running when plugin is loaded */
  virtual bool isSimRunning() const override;

  /* Not used */
  virtual bool isSimPaused() const override;

  /* Always running when plugin is loaded */
  virtual State getState() const override;

private:
  atools::fs::sc::DataCopyFunctionType dataCopyFunc = nullptr;
  bool verbose = false, simPaused = false;
};

} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_XPCONNECTHANDLER_H
