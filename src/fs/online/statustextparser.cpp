/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include "fs/online/statustextparser.h"

#include <QDateTime>
#include <QTextStream>
#include <QDebug>

namespace atools {
namespace fs {
namespace online {

void rollIndex(const QStringList& list, int& index)
{
  index++;
  if(index >= list.size())
    index = 0;
}

// Any line starting with ; or # should be regarded as comments and ignored by the client parser.
// ; IMPORTANT NOTE: to use less bandwidth, please download this file ONE TIME ONLY when
// ;                 your application starts, and load it locally
// ;
// ; Data formats are:
// ;
// ; 120128:NOTCP - used by WhazzUp only
// ; msg0         - message to be displayed at application startup
// ; url0         - URLs where complete data files are available. Please choose one randomly every time
// ; url1         - URLs where servers list data files are available. Please choose one randomly every time
// ; moveto0      - URL where to retrieve a more updated status.txt file that overrides this one
// ; metar0       - URL where to retrieve metar. Invoke it passing a parameter like for example: http://data.satita.net/metar.html?id=KBOS
// ; atis0        - URL where to retrieve atis. Invoke it passing a parameter like for example: http://data.satita.net/atis.html?callsign=BOS_CTR
// ;                WARNING: please don't abuse it. Requests take network bandwidth. If possibile please use atis that are present into satnet-data.txt file
// ; user0        - URL where to retrieve statistics web pages
// ;
// ;
// 120218:NOTCP
// ;
// url0=http://vatsim-data.hardern.net/vatsim-data.txt
// url0=http://wazzup.flightoperationssystem.com/vatsim/vatsim-data.txt
// url0=http://data.vattastic.com/vatsim-data.txt
// url0=http://info.vroute.net/vatsim-data.txt
// ;
// url1=http://vatsim-data.hardern.net/vatsim-servers.txt
// url1=http://wazzup.flightoperationssystem.com/vatsim/vatsim-servers.txt
// url1=http://data.vattastic.com/vatsim-servers.txt
// url1=http://info.vroute.net/vatsim-servers.txt
// ;
// metar0=http://metar.vatsim.net/metar.php
// ;
// atis0=http://stats.vatsim.net/atis.html
// ;
// user0=http://stats.vatsim.net/search_id.php
// ;
// ; END

StatusTextParser::StatusTextParser()
{
}

StatusTextParser::~StatusTextParser()
{

}

void StatusTextParser::read(QString file)
{
  QTextStream stream(&file, QIODevice::ReadOnly | QIODevice::Text);
  read(stream);
}

void StatusTextParser::read(QTextStream& stream)
{
  reset();

  while(!stream.atEnd())
  {
    QString line = stream.readLine().trimmed();

    if(line.isEmpty() || line.startsWith(";") || line.startsWith("#"))
      continue;
    QString key = line.section('=', 0, 0);

    if(key == "url0")
      urlList.append(line.section('=', 1).trimmed());
    if(key == "gzurl0")
      urlListGzip.append(line.section('=', 1).trimmed());
    else if(key == "url1")
      urlListVoice.append(line.section('=', 1).trimmed());
    else if(key == "msg0")
      message = line.section('=', 1).trimmed();
  }
}

void StatusTextParser::reset()
{
  urlList.clear();
  urlListVoice.clear();
  urlListGzip.clear();
  message.clear();
  curUrlIndex = curGzipUrlIndex = curVoiceUrlIndex = 0;
}

QString StatusTextParser::getRandomUrl(bool& gzipped)
{
  if(!urlListGzip.isEmpty())
  {
    gzipped = true;
    rollIndex(urlListGzip, curGzipUrlIndex);
    return urlListGzip.at(curGzipUrlIndex);
  }
  else if(!urlList.isEmpty())
  {
    gzipped = false;
    rollIndex(urlList, curUrlIndex);
    return urlList.at(curUrlIndex);
  }
  else
    return QString();
}

QString StatusTextParser::getRandomVoiceUrl()
{
  if(!urlListVoice.isEmpty())
  {
    rollIndex(urlListVoice, curVoiceUrlIndex);
    return urlListVoice.at(curVoiceUrlIndex);
  }
  else
    return QString();
}

} // namespace online
} // namespace fs
} // namespace atools
