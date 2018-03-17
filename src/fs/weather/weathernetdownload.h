/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FS_WEATHERNETDOWNLOAD_H
#define ATOOLS_FS_WEATHERNETDOWNLOAD_H

#include "geo/simplespatialindex.h"
#include "fs/weather/weathertypes.h"

#include <QNetworkAccessManager>
#include <QTimer>

namespace atools {
namespace fs {
namespace weather {

/*
 * Manages metar files that are download fully from the web like IVAO.
 * Has a timer that triggers a recurrent lookup.
 */
class WeatherNetDownload :
  public QObject
{
  Q_OBJECT

public:
  WeatherNetDownload(QObject *parent);
  virtual ~WeatherNetDownload();

  /* Download text file and emit weatherUpdated when done. Will start update timer. */
  void download();

  /*
   * @return metar from cache or empty if not entry was found in the cache. Once the request was
   * completed the signal weatherUpdated is emitted and calling this method again will return the metar.
   */
  atools::fs::weather::MetarResult getMetar(const QString& airportIcao, const atools::geo::Pos& pos);

  /* Set download request URL */
  void setRequestUrl(const QString& url)
  {
    requestUrl = url;
  }

  /* Re-download every number of seconds and emit weatherUpdated when done. This will start the update timer. */
  void setSetUpdatePeriod(int seconds);

  /* Set to a function that returns the coordinates for an airport ident. Needed to find the nearest. */
  void setFetchAirportCoords(const std::function<atools::geo::Pos(const QString&)>& value)
  {
    fetchAirportCoords = value;
  }

  void startTimer();

signals:
  /* Emitted when file was downloaded and udpated */
  void weatherUpdated();

private:
  void httpFinished();
  void cancelReply();
  void readyRead();
  void parseFile();

  QNetworkAccessManager networkManager;
  QTimer updateTimer;
  QString requestUrl;
  int updatePeriodSeconds = 300;
  QNetworkReply *reply = nullptr;
  std::function<atools::geo::Pos(const QString&)> fetchAirportCoords;

  atools::geo::SimpleSpatialIndex<QString, QString> index;
  QByteArray metarFile;

};

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_WEATHERNETDOWNLOAD_H
