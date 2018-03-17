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

#include "fs/weather/weathertypes.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace atools {
namespace fs {
namespace weather {

bool testUrl(const QString& url, const QString& airportIcao, QString& result)
{
  QNetworkAccessManager network;
  QNetworkRequest request(QUrl(url.arg(airportIcao)));
  QNetworkReply *reply = network.get(request);

  QEventLoop eventLoop;
  QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
  eventLoop.exec();

  if(reply->error() == QNetworkReply::NoError)
  {
    result = reply->readLine();
    result.append(reply->readLine());
    result.append(reply->readLine());
    reply->deleteLater();
    return true;
  }
  else
  {
    result = reply->errorString();
    reply->deleteLater();
    return false;
  }
}

} // namespace weather
} // namespace fs
} // namespace atools
