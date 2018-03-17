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

#ifndef ATOOLS_FS_WEATHERNETWORK_H
#define ATOOLS_FS_WEATHERNETWORK_H

#include "util/timedcache.h"

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>

namespace atools {
namespace fs {
namespace weather {

/*
 * Provides a source of metar data for airports. Supports NOAA and VATSIM like weather services
 * that deliver one weather report per request.
 *
 * Uses hashmaps to cache online requests. Cache entries will timeout after 15 minutes.
 *
 * Only one request is done. If a request is already waiting a new one will cancel the old one.
 */
class WeatherNetSingle :
  public QObject
{
  Q_OBJECT

public:
  WeatherNetSingle(QObject *parent, int timeoutMs);
  virtual ~WeatherNetSingle();

  /*
   * @return metar from cache or empty if not entry was found in the cache. Once the request was
   * completed the signal weatherUpdated is emitted and calling this method again will return the metar.
   */
  QString getMetar(const QString& airportIcao);

  /* Set request URL. %1 is ICAO placeholder */
  void setRequestUrl(const QString& url)
  {
    requestUrl = url;
  }

signals:
  /* Emitted when a request to weather was fullfilled */
  void weatherUpdated();

private:
  void loadMetar(const QString& airportIcao);

  void httpFinished(const QString& icao);
  void httpFinishedMetar();

  void cancelReply();
  void flushRequestQueue();

  atools::util::TimedCache<QString, QString> metarCache;

  QNetworkAccessManager networkManager;

  // Stores the request ICAO so we can send it to httpFinished()
  QString metarRequestIcao;

  // Keeps the reply
  QNetworkReply *replyMetar = nullptr;
  QStringList metarRequests;

  QTimer flushQueueTimer;
  QString requestUrl;

};

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_WEATHERNETWORK_H
