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

#include "fs/weather/weathernetsingle.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QEventLoop>

namespace atools {
namespace fs {
namespace weather {

WeatherNetSingle::WeatherNetSingle(QObject *parent, int timeoutMs)
  : QObject(parent), metarCache(timeoutMs)
{
  connect(&flushQueueTimer, &QTimer::timeout, this, &WeatherNetSingle::flushRequestQueue);

  flushQueueTimer.setInterval(1000);
  flushQueueTimer.start();
}

WeatherNetSingle::~WeatherNetSingle()
{
  flushQueueTimer.stop();

  // Remove any outstanding requests
  cancelReply();
}

QString WeatherNetSingle::getMetar(const QString& airportIcao)
{
  if(!requestUrl.isEmpty())
  {
    QString *metar = metarCache.value(airportIcao);
    if(metar != nullptr)
      return QString(*metar);
    else
    {
      if(!metarRequests.contains(airportIcao))
      {
        metarRequests.append(airportIcao);
        flushRequestQueue();
      }
    }
  }

  return QString();
}

void WeatherNetSingle::flushRequestQueue()
{
  if(!metarRequests.isEmpty() && replyMetar == nullptr && !requestUrl.isEmpty())
  {
    qDebug() << Q_FUNC_INFO << "flushing queue" << metarRequests;

    loadMetar(metarRequests.last());
  }
}

void WeatherNetSingle::cancelReply()
{
  if(replyMetar != nullptr)
  {
    disconnect(replyMetar, &QNetworkReply::finished, this, &WeatherNetSingle::httpFinishedMetar);
    replyMetar->abort();
    replyMetar->deleteLater();
    replyMetar = nullptr;
    metarRequestIcao.clear();
  }
}

void WeatherNetSingle::loadMetar(const QString& airportIcao)
{
  // NOAA
  // http://www.aviationweather.gov/static/adds/metars/stations.txt
  // http://weather.noaa.gov/pub/data/observations/metar/stations/EDDL.TXT
  // http://weather.noaa.gov/pub/data/observations/metar/

  // VATSIM
  // http://metar.vatsim.net/metar.php?id=EDDF

  if(replyMetar != nullptr)
  {
    if(!metarRequests.contains(airportIcao))
    {
      qDebug() << Q_FUNC_INFO << airportIcao;
      metarRequests.append(airportIcao);
    }
  }
  else
  {
    qDebug() << Q_FUNC_INFO << "Building METAR request" << airportIcao << requestUrl;
    cancelReply();

    metarRequestIcao = airportIcao;
    QNetworkRequest request(QUrl(requestUrl.arg(airportIcao)));

    replyMetar = networkManager.get(request);

    if(replyMetar != nullptr)
      connect(replyMetar, &QNetworkReply::finished, this, &WeatherNetSingle::httpFinishedMetar);
    else
      qWarning() << "METAR Reply is null";
  }
}

/* Called by network reply signal */
void WeatherNetSingle::httpFinishedMetar()
{
  qDebug() << Q_FUNC_INFO << metarRequestIcao;

  metarRequests.removeAll(metarRequestIcao);
  httpFinished(metarRequestIcao);
  replyMetar = nullptr;

  if(!metarRequests.isEmpty())
    loadMetar(metarRequests.last());
}

void WeatherNetSingle::httpFinished(const QString& icao)
{
  if(replyMetar != nullptr)
  {
    if(replyMetar->error() == QNetworkReply::NoError)
    {
      QString metar = replyMetar->readAll().simplified();
      if(!metar.contains("no metar available", Qt::CaseInsensitive))
        // Add metar with current time
        metarCache.insert(icao, metar);
      else
        // Add empty record so we know there is no weather station
        metarCache.insert(icao, QString());
      // mainWindow->setStatusMessage(tr("Weather information updated."));
      emit weatherUpdated();
    }
    else if(replyMetar->error() != QNetworkReply::OperationCanceledError)
    {
      metarCache.insert(icao, QString());
      if(replyMetar->error() != QNetworkReply::ContentNotFoundError)
        qWarning() << "Request for" << icao << "failed. Reason:" << replyMetar->errorString();
    }
    replyMetar->abort();
    replyMetar->disconnect(replyMetar, &QNetworkReply::finished, this, &WeatherNetSingle::httpFinishedMetar);
    replyMetar->deleteLater();
  }
}

} // namespace weather
} // namespace fs
} // namespace atools
