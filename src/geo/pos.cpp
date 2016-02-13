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

#include "geo/pos.h"
#include "exception.h"

#include <QRegularExpression>

namespace atools {
namespace geo {

const QString Pos::LONG_FORMAT("%1%2° %3' %4\",%5%6° %7' %8\",%9%10");
const QRegularExpression Pos::LONG_FORMAT_REGEXP(
  "([ns])\\s*([0-9]+)\\s*°\\s*([0-9]+)\\s*'\\s*([0-9\\.]+)\\s*\"\\s*,\\s*"
  "([ew])\\s*([0-9]+)\\s*°\\s*([0-9]+)\\s*'\\s*([0-9\\.]+)\\s*\"\\s*,\\s*"
  "([+-])\\s*([0-9\\.]+)");

Pos::Pos()
  : lonX(0.f), latY(0.f), altitude(0.f)
{
}

Pos::Pos(float longitudeX, float latitudeY, float alt)
  : lonX(longitudeX), latY(latitudeY), altitude(alt)
{
}

Pos::Pos(int lonXDeg,
         int lonXMin,
         float lonXSec,
         bool west,
         int latYDeg,
         int latYMin,
         float latYSec,
         bool south,
         float alt)
{
  latY = (latYDeg + latYMin / 60.f + latYSec / 3600.f) * (south ? -1.f : 1.f);
  lonX = (lonXDeg + lonXMin / 60.f + lonXSec / 3600.f) * (west ? -1.f : 1.f);
  altitude = alt;
}

Pos::Pos(const QString& str)
{
  QRegularExpressionMatch match = LONG_FORMAT_REGEXP.match(str.trimmed().toLower());

  if(match.hasMatch())
  {
    QString ns = match.captured(1);
    int latYDeg = match.captured(2).toInt();
    int latYMin = match.captured(3).toInt();
    float latYSec = match.captured(4).toFloat();

    QString ew = match.captured(5);
    int lonXDeg = match.captured(6).toInt();
    int lonXMin = match.captured(7).toInt();
    float lonXSec = match.captured(8).toFloat();

    QString altSign = match.captured(9);
    QString altNum = match.captured(10);

    altitude = QString(altSign + altNum).toFloat();

    latY = (latYDeg + latYMin / 60.f + latYSec / 3600.f) * (ns == "s" ? -1.f : 1.f);
    lonX = (lonXDeg + lonXMin / 60.f + lonXSec / 3600.f) * (ew == "w" ? -1.f : 1.f);
  }
  else
    throw new Exception("Invalid lat/long format \"" + str + "\"");
}

Pos::~Pos()
{
}

int Pos::getLatYDeg() const
{
  return deg(latY);
}

int Pos::getLatYMin() const
{
  return min(latY);
}

float Pos::getLatYSec() const
{
  return sec(latY);
}

int Pos::getLonXDeg() const
{
  return deg(lonX);
}

int Pos::getLonXMin() const
{
  return min(lonX);
}

float Pos::getLonXSec() const
{
  return sec(lonX);
}

QString Pos::toLongString() const
{
  return LONG_FORMAT.arg(latY > 0 ? "N" : "S").
         arg(std::abs(getLatYDeg())).arg(std::abs(getLatYMin())).arg(std::abs(getLatYSec()), 0, 'f', 2).
         arg(lonX > 0 ? "E" : "W").
         arg(std::abs(getLonXDeg())).arg(std::abs(getLonXMin())).arg(std::abs(getLonXSec()), 0, 'f', 2).
         arg(altitude >= 0 ? "+" : "-").arg(std::abs(altitude), 9, 'f', 2, '0');
}

int Pos::deg(float value) const
{
  float min = (value - static_cast<int>(value)) * 60.f;
  float seconds = (min - static_cast<int>(min)) * 60.f;

  int degrees = static_cast<int>(value);
  int minutes = static_cast<int>(min);

  // Avoid 60 seconds due to rounding up when converting to text
  if(seconds >= 59.99f)
    minutes++;
  if(minutes >= 60)
    degrees++;

  return degrees;
}

int Pos::min(float value) const
{
  float min = (value - static_cast<int>(value)) * 60.f;
  float seconds = (min - static_cast<int>(min)) * 60.f;
  int minutes = static_cast<int>(min);

  // Avoid 60 seconds due to rounding up when converting to text
  if(seconds >= 59.99f)
    minutes++;
  if(minutes >= 60)
    minutes = 0;

  return minutes;
}

float Pos::sec(float value) const
{
  float min = (value - static_cast<int>(value)) * 60.f;
  float seconds = (min - static_cast<int>(min)) * 60.f;

  // Avoid 60 seconds due to rounding up when converting to text
  if(seconds >= 59.99f)
    return 0.f;
  else
    return seconds;
}

QDebug operator<<(QDebug out, const Pos& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << "[lonX " << record.lonX
  << ", latY " << record.latY
  << ", alt " << record.altitude << "]";

  return out;
}

} // namespace geo
} // namespace atools
