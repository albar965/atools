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

#ifndef ATOOLS_FS_SIMCONNECTHANDLER_H
#define ATOOLS_FS_SIMCONNECTHANDLER_H

#include <QtGlobal>
#include <QVector>

#include "geo/pos.h"

namespace atools {
namespace fs {
namespace sc {

class SimConnectData;
class SimConnectAircraft;
class SimConnectHandlerPrivate;
class WeatherRequest;

/* Status of the last operation when fetching data. */
enum State
{
  STATEOK,
  FETCH_ERROR,
  OPEN_ERROR,
  DISCONNECTED,
  SIMCONNECT_EXCEPTION
};

/* Reads data synchronously from Fs simconnect interfaces.
 *  For non windows platforms contains also a simple aircraft simulation. */
class SimConnectHandler
{
public:
  SimConnectHandler(bool verboseLogging = false);
  virtual ~SimConnectHandler();

  /* Connect to fs.. Returns true it successful. */
  bool connect();

  /* Fetch data from simulator. Returns false if no data was retrieved due to paused or not running fs. */
  bool fetchData(atools::fs::sc::SimConnectData& data, int radiusKm);

  void setWeatherRequest(const atools::fs::sc::WeatherRequest& request);
  const atools::fs::sc::WeatherRequest&  getWeatherRequest()const;

  /* true if simulator is running and not stuck in open dialogs. */
  bool isSimRunning() const;

  bool isSimPaused() const;

  /* Get state of last call. */
  sc::State getState() const;

private:
  // Used to all the windows and SimConnect stuff out of the header files
  SimConnectHandlerPrivate *p = nullptr;

};

} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_SIMCONNECTHANDLER_H
