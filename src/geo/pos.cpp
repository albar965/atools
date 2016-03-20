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

#include "geo/calculations.h"
#include "geo/pos.h"
#include "exception.h"

#include <QRegularExpression>

namespace atools {
namespace geo {

const double EARTH_RADIUS_METER = 6371. * 1000.;
const double TOLERANCE = 1e-15;

const QString SHORT_FORMAT("%1,%2,%3");
const QString LONG_FORMAT("%1%2° %3' %4\",%5%6° %7' %8\",%9%10");
const QRegularExpression LONG_FORMAT_REGEXP(
  "([ns])\\s*([0-9]+)\\s*°\\s*([0-9]+)\\s*'\\s*([0-9\\.]+)\\s*\"\\s*,\\s*"
  "([ew])\\s*([0-9]+)\\s*°\\s*([0-9]+)\\s*'\\s*([0-9\\.]+)\\s*\"\\s*,\\s*"
  "([+-])\\s*([0-9\\.]+)");

Pos::Pos()
  : lonX(0.f), latY(0.f), altitude(0.f), valid(false)
{
}

Pos::Pos(const Pos& other)
{
  lonX = other.lonX;
  latY = other.latY;
  altitude = other.altitude;
  valid = other.valid;
}

Pos::Pos(float longitudeX, float latitudeY, float alt)
  : lonX(longitudeX), latY(latitudeY), altitude(alt), valid(true)
{
}

Pos::Pos(double longitudeX, double latitudeY, float alt)
  : altitude(alt), valid(true)
{
  lonX = static_cast<float>(longitudeX);
  latY = static_cast<float>(latitudeY);
}

Pos::Pos(int lonXDeg, int lonXMin, float lonXSec, bool west,
         int latYDeg, int latYMin, float latYSec, bool south, float alt)
{
  lonX = (lonXDeg + lonXMin / 60.f + lonXSec / 3600.f) * (west ? -1.f : 1.f);
  latY = (latYDeg + latYMin / 60.f + latYSec / 3600.f) * (south ? -1.f : 1.f);
  altitude = alt;
  valid = true;
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
    valid = true;
  }
  else
    throw new Exception("Invalid lat/long format \"" + str + "\"");
}

Pos& Pos::operator=(const Pos& other)
{
  lonX = other.lonX;
  latY = other.latY;
  altitude = other.altitude;
  valid = other.valid;
  return *this;
}

bool Pos::operator==(const Pos& other) const
{
  return lonX == other.lonX && latY == other.latY && altitude == other.altitude && valid == other.valid;
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

Pos& Pos::normalize()
{
  lonX = normalizeLonXDeg(lonX);
  latY = normalizeLatYDeg(latY);
  return *this;
}

Pos& Pos::toDeg()
{
  lonX = static_cast<float>(toDegree(lonX));
  latY = static_cast<float>(toDegree(latY));
  return *this;
}

Pos& Pos::toRad()
{
  lonX = static_cast<float>(toRadians(lonX));
  latY = static_cast<float>(toRadians(latY));
  return *this;
}

Pos Pos::endpoint(float distanceMeter, float angleDeg) const
{
  return calculateEndpoint(*this, distanceMeter, angleDeg).normalize();
}

float Pos::distanceSimpleTo(const Pos& otherPos) const
{
  return std::abs(lonX - otherPos.lonX) + std::abs(latY - otherPos.latY);
}

float Pos::distanceMeterTo(const Pos& otherPos) const
{
  return static_cast<float>(calculateDistance(*this, otherPos));
}

float Pos::angleDegTo(const Pos& otherPos) const
{
  return static_cast<float>(calculateAngle(*this, otherPos));
}

Pos Pos::endpointRhumb(float distanceMeter, float angleDeg) const
{
  double lon1 = toRadians(getLonX());
  double lat1 = toRadians(getLatY());

  double distanceRad = nmToRad(meterToNm(distanceMeter));
  double tc = toRadians(-angleDeg + 360.);

  double lat = lat1 + distanceRad *cos(tc);
  if(std::abs(lat) > M_PI / 2.)
    return atools::geo::Pos();   // distance too long - return invalid pos

  double q, dphi;
  if(std::abs(lat - lat1) < sqrt(TOLERANCE))
    q = cos(lat1);
  else
  {
    dphi = log(tan(lat / 2. + M_PI / 4.) / tan(lat1 / 2. + M_PI / 4.));
    q = (lat - lat1) / dphi;
  }
  double dlon = -distanceRad *sin(tc) / q;
  double lon = remainder(lon1 + dlon + M_PI, 2. * M_PI) - M_PI;
  return atools::geo::Pos(lon, lat).toDeg().normalize();
}

float Pos::angleDegToRhumb(const Pos& otherPos) const
{
  double lon1 = toRadians(getLonX());
  double lat1 = toRadians(getLatY());
  double lon2 = toRadians(otherPos.getLonX());
  double lat2 = toRadians(otherPos.getLatY());

  double dlonWest = remainder(lon2 - lon1, 2. * M_PI);
  double dlonEast = remainder(lon1 - lon2, 2. * M_PI);
  double dphi = log(tan(lat2 / 2. + M_PI / 4.) / tan(lat1 / 2. + M_PI / 4.));

  double tc;
  if(dlonWest < dlonEast)
    // To west is shorter
    tc = remainder(atan2(-dlonWest, dphi), 2. * M_PI);
  else
    tc = remainder(atan2(dlonEast, dphi), 2. * M_PI);
  return static_cast<float>(atools::geo::normalizeCourse(-atools::geo::toDegree(tc) + 360.));
}

float Pos::distanceMeterToRhumb(const Pos& otherPos) const
{
  double lon1 = toRadians(getLonX());
  double lat1 = toRadians(getLatY());
  double lon2 = toRadians(otherPos.getLonX());
  double lat2 = toRadians(otherPos.getLatY());

  double dlonWest = remainder(lon2 - lon1, 2. * M_PI);
  double dlonEast = remainder(lon1 - lon2, 2. * M_PI);
  double q, distance;
  double dphi = log(tan(lat2 / 2. + M_PI / 4.) / tan(lat1 / 2. + M_PI / 4.));

  if(std::abs(lat2 - lat1) < sqrt(TOLERANCE))
    q = cos(lat1);
  else
    q = (lat2 - lat1) / dphi;

  if(dlonWest < dlonEast)
    // To west is shorter
    distance = sqrt(q * q * dlonWest * dlonWest + (lat2 - lat1) * (lat2 - lat1));
  else
    distance = sqrt(q * q * dlonEast * dlonEast + (lat2 - lat1) * (lat2 - lat1));

  return static_cast<float>(nmToMeter(((180. * 60.) / M_PI) * distance));
}

Pos Pos::interpolateRhumb(const atools::geo::Pos& otherPos, float distanceMeter, float fraction) const
{
  return endpointRhumb(distanceMeter * fraction, angleDegToRhumb(otherPos));
}

Pos Pos::interpolate(const atools::geo::Pos& otherPos, float distanceMeter, float fraction) const
{
  double lon1 = toRadians(getLonX());
  double lat1 = toRadians(getLatY());
  double lon2 = toRadians(otherPos.getLonX());
  double lat2 = toRadians(otherPos.getLatY());
  double distanceRad = nmToRad(meterToNm(distanceMeter));

  double A = sin((1. - fraction) * distanceRad) / sin(distanceRad);
  double B = sin(fraction * distanceRad) / sin(distanceRad);
  double x = A * cos(lat1) * cos(lon1) + B *cos(lat2) * cos(lon2);
  double y = A * cos(lat1) * sin(lon1) + B *cos(lat2) * sin(lon2);
  double z = A * sin(lat1) + B *sin(lat2);
  double lat = atan2(z, sqrt(x * x + y * y));
  double lon = atan2(y, x);
  return atools::geo::Pos(lon, lat).toDeg().normalize();
}

QString Pos::toLongString() const
{
  if(!valid)
    return "Invalid Pos";

  return LONG_FORMAT.arg(latY > 0 ? "N" : "S").
         arg(std::abs(getLatYDeg())).arg(std::abs(getLatYMin())).arg(std::abs(getLatYSec()), 0, 'f', 2).
         arg(lonX > 0 ? "E" : "W").
         arg(std::abs(getLonXDeg())).arg(std::abs(getLonXMin())).arg(std::abs(getLonXSec()), 0, 'f', 2).
         arg(altitude >= 0 ? "+" : "-").arg(std::abs(altitude), 9, 'f', 2, '0');
}

QString Pos::toString() const
{
  if(!valid)
    return "Invalid Pos";

  return SHORT_FORMAT.arg(lonX).arg(latY).arg(altitude);
}

bool Pos::isPole() const
{
  return latY > 89. || latY < -89.;
}

double Pos::calculateAngle(double lonX1, double latY1, double lonX2, double latY2) const
{
  // http://williams.best.vwh.net/avform.htm
  double angle = toDegree(atan2(sin(toRadians(lonX1 - lonX2)) * cos(toRadians(latY2)),
                                cos(toRadians(latY1)) * sin(toRadians(latY2)) -
                                sin(toRadians(latY1)) * cos(toRadians(latY2)) *
                                cos(toRadians(lonX1 - lonX2))));

  return -angle + 360.;
}

double Pos::calculateAngle(const Pos& p1, const Pos& p2) const
{
  return calculateAngle(p1.getLonX(), p1.getLatY(), p2.getLonX(), p2.getLatY());
}

double Pos::calculateDistance(double lonX1, double latY1, double lonX2, double latY2) const
{
  // http://williams.best.vwh.net/avform.htm
  double l1 = (sin((toRadians(latY1 - latY2)) / 2));
  double l2 = (sin((toRadians(lonX1 - lonX2)) / 2));
  double d = 2 * asin(sqrt(l1 * l1 + cos(toRadians(latY1)) * cos(toRadians(latY2)) * l2 * l2));

  return EARTH_RADIUS_METER * d;
}

double Pos::calculateDistance(const Pos& p1, const Pos& p2) const
{
  return calculateDistance(p1.getLonX(), p1.getLatY(), p2.getLonX(), p2.getLatY());
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
  out.nospace().noquote() << "Pos[" << record.toString() << "]";
  return out;
}

Pos atools::geo::Pos::calculateEndpoint(double longitude, double latitude, double dist, double angle) const
{
  // http://williams.best.vwh.net/avform.htm
  double distanceRad = M_PI / (180. * 60.) * meterToNm(dist);
  double angleRad = toRadians(-angle + 360.);
  double latRad = toRadians(latitude);
  double lonRad = toRadians(longitude);

  double lat = asin(sin(latRad) * cos(distanceRad) + cos(latRad) *
                    sin(distanceRad) * cos(angleRad));

  double dlon = atan2(sin(angleRad) * sin(distanceRad) * cos(latRad),
                      cos(distanceRad) - sin(latRad) * sin(lat));
  double lon = remainder(lonRad - dlon + M_PI, 2 * M_PI) - M_PI;

  return Pos(static_cast<float>(toDegree(lon)), static_cast<float>(toDegree(lat)));
}

Pos Pos::calculateEndpoint(const Pos& p, double dist, double angle) const
{
  return calculateEndpoint(p.getLonX(), p.getLatY(), dist, angle);
}

} // namespace geo
} // namespace atools
