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

#ifndef ATOOLS_FS_SIMCONNECTHANDLER_H
#define ATOOLS_FS_SIMCONNECTHANDLER_H

#include <QtGlobal>
#include <QVector>

#include "fs/sc/simconnecttypes.h"
#include "fs/sc/connecthandler.h"

namespace atools {
namespace fs {
namespace sc {

class SimConnectAircraft;
class SimConnectHandlerPrivate;

/* Reads data synchronously from Fs simconnect interfaces.
 *  For non windows platforms contains also a simple aircraft simulation. */
class SimConnectHandler :
  public atools::fs::sc::ConnectHandler
{
public:
  SimConnectHandler(bool verboseLogging = false);
  virtual ~SimConnectHandler() override;

  /* Activate context and load SimConnect DLL */
  bool loadSimConnect(const QString& manifestPath);
  virtual bool isLoaded() const override;

  /* Connect to fs.. Returns true if successful. */
  virtual bool connect() override;

  /* Fetch data from simulator. Returns false if no data was retrieved due to paused or not running fs. */
  virtual bool fetchData(atools::fs::sc::SimConnectData& data, int radiusKm, atools::fs::sc::Options options) override;
  virtual bool fetchWeatherData(atools::fs::sc::SimConnectData& data) override;

  virtual void addWeatherRequest(const atools::fs::sc::WeatherRequest& request) override;
  virtual const atools::fs::sc::WeatherRequest& getWeatherRequest() const override;

  /* true if simulator is running and not stuck in open dialogs. */
  virtual bool isSimRunning() const override;

  virtual bool isSimPaused() const override;

  virtual bool canFetchWeather() const override;

  /* Get state of last call. */
  virtual sc::State getState() const override;

  virtual QString getName() const override;

private:
  // Used to all the windows and SimConnect stuff out of the header files
  SimConnectHandlerPrivate *p = nullptr;
  QByteArray appName;

};

} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_SIMCONNECTHANDLER_H
