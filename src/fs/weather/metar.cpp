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

#include "fs/weather/metar.h"

#include "fs/weather/metarparser.h"

#include <QStringList>
#include <QDebug>

namespace atools {
namespace fs {
namespace weather {

Metar::Metar(const QString& metarString, bool isSimFormat)
  : metar(metarString), simFormat(isSimFormat)
{

}

Metar::~Metar()
{

}

// K53S&A1 000000Z 24705G06KT&D975NG 13520KT&A1528NG 129V141 9999 2ST025&ST001FNVN002N
// 1CI312&CI001FNVN002N 13/12 07/05&A1528 Q1009 @@@ 50 7 135 20 |
QString Metar::getCleanMetar() const
{
  if(simFormat)
  {
    QStringList met = metar.section("@@@", 0, 0).simplified().split(" ");
    QStringList retval;

    for(const QString& str : met)
      retval.append(str.section("&", 0, 0));

    return retval.join(" ");
  }
  else
    return metar;
}

MetarParser Metar::getParsedMetar() const
{
  return MetarParser(getCleanMetar());
}

} // namespace weather
} // namespace fs
} // namespace atools
