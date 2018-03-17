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

#include "fs/weather/weathernetdownload.h"
#include "exception.h"

#include <QNetworkReply>

namespace atools {
namespace fs {
namespace weather {

WeatherNetDownload::WeatherNetDownload(QObject *parent)
  : QObject(parent), index(5000)
{
  connect(&updateTimer, &QTimer::timeout, this, &WeatherNetDownload::download);
}

WeatherNetDownload::~WeatherNetDownload()
{
  updateTimer.stop();
  cancelReply();
}

void WeatherNetDownload::download()
{
  cancelReply();

  QNetworkRequest request((QUrl(requestUrl)));

  reply = networkManager.get(request);

  if(reply != nullptr)
  {
    connect(reply, &QNetworkReply::finished, this, &WeatherNetDownload::httpFinished);
    connect(reply, &QNetworkReply::readyRead, this, &WeatherNetDownload::readyRead);
  }
  else
    qWarning() << "METAR Reply is null";

}

atools::fs::weather::MetarResult WeatherNetDownload::getMetar(const QString& airportIcao, const atools::geo::Pos& pos)
{
  atools::fs::weather::MetarResult result;
  result.requestIdent = airportIcao;
  result.requestPos = pos;

  QString data;
  QString foundKey = index.getTypeOrNearest(data, airportIcao, pos);
  if(!foundKey.isEmpty())
  {
    if(foundKey == airportIcao)
      result.metarForStation = data;
    else
      result.metarForNearest = data;
  }

  result.timestamp = QDateTime::currentDateTime();
  return result;
}

void WeatherNetDownload::startTimer()
{
  updateTimer.setInterval(updatePeriodSeconds * 1000);
  updateTimer.start();
}

void WeatherNetDownload::setSetUpdatePeriod(int seconds)
{
  updatePeriodSeconds = seconds;
  startTimer();
}

void WeatherNetDownload::cancelReply()
{
  if(reply != nullptr)
  {
    disconnect(reply, &QNetworkReply::finished, this, &WeatherNetDownload::httpFinished);
    reply->abort();
    reply->deleteLater();
    reply = nullptr;
  }
}

void WeatherNetDownload::httpFinished()
{
  qDebug() << Q_FUNC_INFO;

  if(reply != nullptr)
  {
    metarFile.append(reply->readAll());

    if(reply->error() == QNetworkReply::NoError)
    {
      parseFile();
      startTimer();
      emit weatherUpdated();
    }
    else
    {
      qWarning() << Q_FUNC_INFO << "Error downloading IVAO metar file" << reply->errorString();
      // throw atools::Exception(tr("Unable to download METAR file from \"%1\"").arg(requestUrl));
    }

    cancelReply();
  }
}

void WeatherNetDownload::readyRead()
{
  if(reply != nullptr)
  {
    qint64 byteAvailable = reply->bytesAvailable();
    QByteArray bytes = reply->read(byteAvailable);

    metarFile.append(bytes);

    if(reply->error() != QNetworkReply::NoError)
      qWarning() << Q_FUNC_INFO << "Error downloading IVAO metar file" << reply->errorString();
  }
}

// AGGH 161200Z 14002KT 9999 FEW016 25/24 Q1010
// AYNZ 160800Z 09005G10KT 9999 SCT030 BKN ABV050 27/24 Q1007 RMK
// AYPY 160700Z 28010KT 9999 SCT025 OVC050 28/23 Q1008 RMK/ BUILD UPS TO S/W
void WeatherNetDownload::parseFile()
{
  QTextStream stream(&metarFile, QIODevice::ReadOnly | QIODevice::Text);

  while(!stream.atEnd())
  {
    QString line = stream.readLine().simplified();
    QString ident = line.section(' ', 0, 0);

    atools::geo::Pos pos = fetchAirportCoords(ident);

    if(pos.isValid())
      index.insert(ident, line, pos);
  }
  qDebug() << Q_FUNC_INFO << "Loaded" << index.size() << "metars from" << requestUrl;
}

} // namespace weather
} // namespace fs
} // namespace atools
