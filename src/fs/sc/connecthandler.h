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

#ifndef ATOOLS_FS_CONNECTHANDLER_H
#define ATOOLS_FS_CONNECTHANDLER_H

#include "fs/sc/simconnecttypes.h"

namespace atools {
namespace fs {
namespace sc {

class SimConnectData;
class WeatherRequest;

/* Status of the last operation when fetching data. */
enum State
{
  STATEOK,
  FETCH_ERROR,
  OPEN_ERROR,
  DISCONNECTED,
  EXCEPTION
};

/*
 * Abstract interface for simulator connection interfaces
 * Reads data synchronously from Fs simconnect interfaces or inside a X-Plane plugin.
 * For non windows platforms contains also a simple aircraft simulation.
 */
class ConnectHandler
{
public:
  ConnectHandler();
  virtual ~ConnectHandler();

  /* Connect to fs.. Returns true if successful. */
  virtual bool connect() = 0;

  virtual bool isLoaded() const = 0;

  /* Fetch data from simulator. Returns false if no data was retrieved due to paused or not running fs. */
  virtual bool fetchData(atools::fs::sc::SimConnectData& data, int radiusKm, atools::fs::sc::Options options) = 0;
  virtual bool fetchWeatherData(atools::fs::sc::SimConnectData& data) = 0;

  virtual void addWeatherRequest(const atools::fs::sc::WeatherRequest& request) = 0;
  virtual const atools::fs::sc::WeatherRequest& getWeatherRequest() const = 0;

  /* true if simulator is running and not stuck in open dialogs. */
  virtual bool isSimRunning() const = 0;

  virtual bool isSimPaused() const = 0;

  virtual bool canFetchWeather() const = 0;

  /* Get state of last call. */
  virtual sc::State getState() const = 0;

  /* Name which can be used when saving options */
  virtual QString getName() const = 0;

};

} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_CONNECTHANDLER_H
