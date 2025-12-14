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

#include "fs/weather/weathertypes.h"

#include <QEventLoop>
#include <QFileInfo>
#include <QNetworkReply>

namespace atools {
namespace fs {
namespace weather {

bool testUrl(QStringList& result, const QString& urlStr, const QString& airportIcao, const QHash<QString, QString>& headerParameters,
             int readLines)
{
  if(urlStr.startsWith("http://", Qt::CaseInsensitive) || urlStr.startsWith("https://", Qt::CaseInsensitive))
  {
    QNetworkAccessManager network;
    QNetworkRequest request(QUrl(!airportIcao.isEmpty() && urlStr.contains("%1") ? urlStr.arg(airportIcao) : urlStr));

    for(auto it = headerParameters.begin(); it != headerParameters.end(); ++it)
      request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());

    QNetworkReply *reply = network.get(request);

    QEventLoop eventLoop;
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    if(reply->error() == QNetworkReply::NoError)
    {
      for(int i = 0; i <= readLines && !reply->atEnd(); i++)
        result.append(reply->readLine().mid(0, 80));
      reply->deleteLater();
      return true;
    }
    else
    {
      result.append(reply->errorString());
      reply->deleteLater();
    }
  }
  else
  {
    QFileInfo fi(urlStr);

    if(fi.exists())
    {
      if(fi.isFile())
      {
        QFile file(urlStr);
        if(file.open(QIODevice::Text | QIODevice::ReadOnly))
        {
          QTextStream stream(&file);
          for(int i = 0; i <= readLines && !stream.atEnd(); i++)
            result.append(stream.readLine().mid(0, 80));
          file.close();
          return true;
        }
        else
          result.append(QObject::tr("Cannot open file \"%1\". Reason: %2.").
                        arg(urlStr).arg(file.errorString()));
      }
      else
        result.append(QObject::tr("Cannot open file \"%1\". Reason: %2.").
                      arg(urlStr).arg(QObject::tr("Is not a file.")));
    }
    else
      result.append(QObject::tr("Cannot open file \"%1\". Reason: %2.").
                    arg(urlStr).arg(QObject::tr("File does not exist")));
  }
  return false;
}

} // namespace weather
} // namespace fs
} // namespace atools
