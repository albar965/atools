/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#include <QSharedMemory>
#include <functional>

namespace atools {
namespace fs {
namespace sc {

/* Defaul size of the shared memory segment */
static const int SHARED_MEMORY_SIZE = 8196;
static const QLatin1Literal SHARED_MEMORY_KEY("LittleXpconnect");

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
  XpConnectHandler();
  virtual ~XpConnectHandler();

  /* Attach to shared memory if available */
  virtual bool connect() override;

  /* Always loaded since X-Plane is always available */
  virtual bool isLoaded() const override;

  /* Fetch data from the shared memory. */
  virtual bool fetchData(SimConnectData& data, int radiusKm, Options options) override;

  /* Not supported in X-Plane */
  virtual bool fetchWeatherData(SimConnectData& data) override;

  /* Not supported in X-Plane */
  virtual void addWeatherRequest(const WeatherRequest& request) override;

  /* Not supported in X-Plane */
  virtual const WeatherRequest& getWeatherRequest() const override;

  /* If state is ok and attached to shared memory */
  virtual bool isSimRunning() const override;

  /* Not used */
  virtual bool isSimPaused() const override;

  /* State shows if we are attached or not */
  virtual atools::fs::sc::State getState() const override;

  /* Symbolic name for logging */
  QString getName() const override;

private:
  void disconnect();

  QSharedMemory sharedMemory;
  atools::fs::sc::State state = DISCONNECTED;
};

} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_XPCONNECTHANDLER_H
