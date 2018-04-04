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
#include "fs/weather/weathertypes.h"

#include "util/httpdownloader.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QEventLoop>

using atools::util::HttpDownloader;

namespace atools {
namespace fs {
namespace weather {

WeatherNetSingle::WeatherNetSingle(QObject *parent, int timeoutMs, bool verboseLogging)
  : QObject(parent), metarCache(timeoutMs), index(5000), verbose(verboseLogging)
{
  connect(&flushQueueTimer, &QTimer::timeout, this, &WeatherNetSingle::flushRequestQueue);

  flushQueueTimer.setInterval(1000);
  flushQueueTimer.start();
}

WeatherNetSingle::~WeatherNetSingle()
{
  flushQueueTimer.stop();
  delete indexDownloader;

  // Remove any outstanding requests
  cancelReply();
}

MetarResult WeatherNetSingle::getMetar(const QString& airportIcao, const geo::Pos& pos)
{
  // Load the web page containing the index to all files
  loadIndex();

  atools::fs::weather::MetarResult result;
  result.requestIdent = airportIcao;
  result.requestPos = pos;

  if(stationIndexUrl.isEmpty())
    // Do not use downloaded index - try direct
    result.metarForStation = getMetarInternal(airportIcao);
  else
  {
    // This will rely on loadIndex()
    QString foundKey = index.getTypeOrNearest(airportIcao, pos);
    if(!foundKey.isEmpty())
    {
      if(foundKey == airportIcao)
        result.metarForStation = getMetarInternal(airportIcao);
      else
        result.metarForNearest = getMetarInternal(foundKey);
    }
  }

  result.timestamp = QDateTime::currentDateTime();
  return result;
}

QString WeatherNetSingle::getMetarInternal(const QString& airportIcao)
{
  if(!requestUrl.isEmpty() && (stationIndex.isEmpty() || stationIndex.contains(airportIcao)))
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

void WeatherNetSingle::setStationIndexUrl(const QString& url,
                                          const std::function<void(QString & icao, QDateTime & lastUpdate,
                                                                   const QString &line)>& parseFunc)
{
  delete indexDownloader;
  indexDownloader = nullptr;

  stationIndexUrl = url;
  indexParseFunction = parseFunc;
}

void WeatherNetSingle::flushRequestQueue()
{
  if(!metarRequests.isEmpty() && replyMetar == nullptr && !requestUrl.isEmpty())
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << "flushing queue" << metarRequests;

    loadMetar(metarRequests.last());
  }
}

void WeatherNetSingle::loadIndex()
{
  // Trigger download of the index page
  if(stationIndex.isEmpty() && !stationIndexUrl.isEmpty() && indexParseFunction != nullptr &&
     indexDownloader == nullptr)
  {
    indexDownloader = new HttpDownloader(parent(), verbose);
    indexDownloader->setUrl(stationIndexUrl);

    connect(indexDownloader, &HttpDownloader::downloadFinished, this, &WeatherNetSingle::indexDownloadFinished);

    indexDownloader->startDownload();
  }
}

void WeatherNetSingle::indexDownloadFinished(const QByteArray& data, QString downloadUrl)
{
  QTextStream stream(data, QIODevice::ReadOnly | QIODevice::Text);

  // Maximum age is six hours
  QDateTime old = QDateTime::currentDateTimeUtc().addSecs(-3600 * 6);
  stationIndex.clear();

  // Read and parse the (HTML) page
  while(!stream.atEnd())
  {
    QString line = stream.readLine().simplified();
    QString icao;
    QDateTime datetime;
    indexParseFunction(icao, datetime, line);

    if(!icao.isEmpty() && fetchAirportCoords != nullptr && (!datetime.isValid() || datetime > old))
    {
      atools::geo::Pos pos = fetchAirportCoords(icao);

      if(pos.isValid())
      {
        index.insert(icao, pos);
        stationIndex.insert(icao);
      }
    }
  }

  if(verbose)
    qDebug() << Q_FUNC_INFO << "Loaded" << data.size() << "bytes and" << stationIndex.size()
             << "metars from" << downloadUrl;

  emit weatherUpdated();
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
      if(verbose)
        qDebug() << Q_FUNC_INFO << airportIcao;
      metarRequests.append(airportIcao);
    }
  }
  else
  {
    if(verbose)
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
  if(verbose)
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
