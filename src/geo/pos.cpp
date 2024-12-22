/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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
#include "geo/point3d.h"
#include "exception.h"
#include "atools.h"
#include "geo/linestring.h"

#include <QDataStream>
#include <QRegularExpression>

namespace atools {
namespace geo {

const static QString OVERFLOW_60_TEST("%1");
const static QString OVERFLOW_60_TEST_TEXT("60");
const static float MAX_SECONDS = 59.98f;

const static QString SHORT_FORMAT("%1,%2");
const static QString SHORT_FORMAT_ALT("%1,%2,%3");
const static QString HUMAN_FORMAT("%6° %7' %8\"%5, %2° %3' %4\"%1");

/* FSX and MSFS formats */
const static QString LONG_FORMAT("%1%2° %3' %4\",%5%6° %7' %8\",%9%10");
const static QString LONG_FORMAT_STAR("%1%2* %3' %4\",%5%6* %7' %8\",%9%10");

// This is the format that is used in FSX flight plans
// N49° 26' 41.57",E9° 12' 5.49",+005500.00
// N49* 26' 41.57",E9* 12' 5.49"
const static QRegularExpression LONG_FORMAT_REGEXP(
  "([ns])\\s*([0-9]+)\\s*[°\\*]\\s*([0-9]+)\\s*'\\s*([0-9\\.]+)\\s*\"\\s*,\\s*"
  "([ew])\\s*([0-9]+)\\s*[°\\*]\\s*([0-9]+)\\s*'\\s*([0-9\\.]+)\\s*\"\\s*,?\\s*"
  "([+-]?)\\s*([0-9\\.]+)?");

// Skyvector to FSX export tool
// 54.765892 , -130.647858,+5000
const static QRegularExpression LONG_FORMAT_REGEXP_SIGNED("([0-9\\.\\-\\+]+)\\s*,\\s*"
                                                          "([0-9\\.\\-\\+]+)\\s*,\\s*"
                                                          "([0-9\\.\\-\\+]+)");

// Format as used in FS9 flight plans
// N54* 16.82', W008* 35.95', +000011.00
const static QRegularExpression LONG_FORMAT_OLD_REGEXP(
  "([ns])\\s*([0-9]+)\\s*[\\*°]\\s*([0-9\\.]+)\\s*[']?\\s*'\\s*,\\s*"
  "([ew])\\s*([0-9]+)\\s*[\\*°]\\s*([0-9\\.]+)\\s*[']?\\s*'\\s*,\\s*"
  "([+-]?)\\s*([0-9\\.]+)?");

using atools::absInt;

Pos::Pos(const QVariant& longitudeX, const QVariant& latitudeY, const QVariant& alt)
{
  if(!longitudeX.isNull() && !latitudeY.isNull() && longitudeX.isValid() && latitudeY.isValid())
  {
    *this = Pos(longitudeX.toFloat(), latitudeY.toFloat());

    if(!alt.isNull() && alt.isValid())
      altitude = alt.toFloat();
  }
  else
  {
    lonX = INVALID_VALUE;
    latY = INVALID_VALUE;
    altitude = 0.f;
  }
}

Pos::Pos(int lonXDeg, int lonXMin, float lonXSec, bool west,
         int latYDeg, int latYMin, float latYSec, bool south, float alt)
{
  lonX = (lonXDeg + lonXMin / 60.f + lonXSec / 3600.f) * (west ? -1.f : 1.f);
  latY = (latYDeg + latYMin / 60.f + latYSec / 3600.f) * (south ? -1.f : 1.f);
  altitude = alt;
}

Pos::Pos(const QString& str, bool errorOnInvalid)
  : lonX(INVALID_VALUE), latY(INVALID_VALUE), altitude(0.f)
{
  QString coordStr(str.simplified().toLower());

  if(coordStr.isEmpty() && errorOnInvalid)
    throw Exception("Coordinate string is empty");

  QRegularExpressionMatch match;

  if((match = LONG_FORMAT_REGEXP.match(coordStr)).hasMatch())
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
  else if((match = LONG_FORMAT_REGEXP_SIGNED.match(coordStr)).hasMatch())
  {
    latY = match.captured(1).toFloat();
    lonX = match.captured(2).toFloat();
    altitude = match.captured(3).toFloat();
  }
  else if((match = LONG_FORMAT_OLD_REGEXP.match(coordStr)).hasMatch())
  {
    QString ns = match.captured(1);
    int latYDeg = match.captured(2).toInt();
    float latYMin = match.captured(3).toFloat();

    QString ew = match.captured(4);
    int lonXDeg = match.captured(5).toInt();
    float lonXMin = match.captured(6).toFloat();

    QString altSign = match.captured(7);
    QString altNum = match.captured(8);

    altitude = QString(altSign + altNum).toFloat();

    latY = (latYDeg + latYMin / 60.f) * (ns == "s" ? -1.f : 1.f);
    lonX = (lonXDeg + lonXMin / 60.f) * (ew == "w" ? -1.f : 1.f);
  }
  else if(errorOnInvalid)
    throw Exception("Invalid lat/long format \"" + str + "\"");
}

bool Pos::almostEqual(const Pos& other, float epsilon) const
{
  return atools::almostEqual(lonX, other.lonX, epsilon) && atools::almostEqual(latY, other.latY, epsilon);
}

float Pos::getLatYRad() const
{
  return toRadians(latY);
}

float Pos::getLonXRad() const
{
  return toRadians(lonX);
}

bool Pos::nearGridLonLat(float spacingLonX, float spacingLatY, float epsilon) const
{
  return atools::almostEqual(lonX, atools::roundToNearest(lonX, spacingLonX), epsilon) &&
         atools::almostEqual(latY, atools::roundToNearest(latY, spacingLatY), epsilon);
}

Pos& Pos::snapToGrid()
{
  lonX = std::round(lonX);
  latY = std::round(latY);
  return *this;
}

Pos& Pos::normalize()
{
  if(isValid())
  {
    lonX = normalizeLonXDeg(lonX);
    latY = normalizeLatYDeg(latY);
  }
  return *this;
}

Pos Pos::normalized() const
{
  Pos retval(*this);
  return retval.normalize();
}

Pos Pos::alt(float alt) const
{
  Pos retval(*this);
  retval.setAltitude(alt);
  return retval;
}

Pos& Pos::toDeg()
{
  if(isValid())
  {
    lonX = toDegree(lonX);
    latY = toDegree(latY);
  }
  return *this;
}

Pos& Pos::toRad()
{
  if(isValid())
  {
    lonX = toRadians(lonX);
    latY = toRadians(latY);
  }
  return *this;
}

void Pos::swap(Pos& other)
{
  std::swap(lonX, other.lonX);
  std::swap(latY, other.latY);
  std::swap(altitude, other.altitude);
}

void endpointRad(double lonX, double latY, double distance, double angle, double& endLonX, double& endLatY)
{
  endLatY = asin(sin(latY) * cos(distance) + cos(latY) * sin(distance) * cos(angle));

  double dlon = atan2(sin(angle) * sin(distance) * cos(latY), cos(distance) - sin(latY) * sin(endLatY));
  endLonX = remainder(lonX - dlon + M_PI, 2 * M_PI) - M_PI;
}

Pos Pos::endpoint(float distanceMeter, float angleDeg) const
{
  if(!isValid())
    return EMPTY_POS;

  if(distanceMeter == 0.f)
    return *this;

  double lon, lat;
  endpointRad(toRadians(static_cast<double>(lonX)), toRadians(static_cast<double>(latY)),
              meterToRad(static_cast<double>(distanceMeter)), toRadians(
                -static_cast<double>(angleDeg) + 360.), lon, lat);

  return Pos(static_cast<float>(toDegree(lon)), static_cast<float>(toDegree(lat))).normalize();
}

Pos Pos::endpointDouble(double distanceMeter, double angleDeg) const
{
  if(!isValid())
    return EMPTY_POS;

  if(distanceMeter == 0.)
    return *this;

  double lon, lat;
  endpointRad(toRadians(lonX), toRadians(latY), meterToRad(distanceMeter), toRadians(-angleDeg + 360.), lon, lat);

  return Pos(static_cast<float>(toDegree(lon)), static_cast<float>(toDegree(lat))).normalize();
}

float Pos::distanceSimpleTo(const Pos& otherPos) const
{
  if(!isValid() || !otherPos.isValid())
    return INVALID_VALUE;
  else if(*this == otherPos)
    return 0.f;
  else
    return std::abs(lonX - otherPos.lonX) + std::abs(latY - otherPos.latY);
}

double Pos::distanceMeterToDouble(const Pos& otherPos) const
{
  if(!isValid() || !otherPos.isValid())
    return INVALID_VALUE;
  else if(*this == otherPos)
    return 0.;
  else
    return distanceRad(toRadians(static_cast<double>(lonX)),
                       toRadians(static_cast<double>(latY)),
                       toRadians(static_cast<double>(otherPos.lonX)),
                       toRadians(static_cast<double>(otherPos.latY))) * EARTH_RADIUS_METER_DOUBLE;
}

double Pos::distanceMeterTo3dDouble(const Pos& otherPos, double altitudeWeight) const
{
  double distMeter = distanceMeterToDouble(otherPos);

  if(altitude < INVALID_VALUE / 2. && otherPos.altitude < INVALID_VALUE / 2.)
  {
    double altDiffMeter = feetToMeter(std::abs(altitude - otherPos.altitude)) * altitudeWeight;

    if(atools::almostNotEqual(altDiffMeter, 0.))
      return std::sqrt(distMeter * distMeter + altDiffMeter * altDiffMeter);
  }
  return distMeter;
}

void Pos::distanceMeterToLine(const Pos& pos1, const Pos& pos2, LineDistance& result) const
{
  result.distance = result.distanceFrom1 = result.distanceFrom2 = INVALID_VALUE;
  result.status = INVALID;

  if(!isValid() || !pos1.isValid() || !pos2.isValid())
    return;

  if(pos1 == pos2)
  {
    result.status = ALONG_TRACK;
    result.distance = distanceMeterTo(pos1);
    result.distanceFrom1 = result.distanceFrom2 = 0.f;
    return;
  }

  Pos p = *this;
  p.toRad();
  Pos p1 = pos1;
  p1.toRad();
  Pos p2 = pos2;
  p2.toRad();

  double dist1To2 = distanceRad(p1.lonX, p1.latY, p2.lonX, p2.latY);

  if(*this == pos1)
  {
    result.status = ALONG_TRACK;
    result.distance = 0.f;
    result.distanceFrom1 = 0.f;
    result.distanceFrom2 = static_cast<float>(dist1To2 * EARTH_RADIUS_METER_DOUBLE);
    return;
  }

  if(*this == pos2)
  {
    result.status = ALONG_TRACK;
    result.distance = 0.f;
    result.distanceFrom1 = static_cast<float>(dist1To2 * EARTH_RADIUS_METER_DOUBLE);
    result.distanceFrom2 = 0.f;
    return;
  }

  double courseFrom1 = courseRad(p1.lonX, p1.latY, p.lonX, p.latY);
  double course1To2 = courseRad(p1.lonX, p1.latY, p2.lonX, p2.latY);
  double distFrom1 = distanceRad(p1.lonX, p1.latY, p.lonX, p.latY);
  double distFrom2 = distanceRad(p2.lonX, p2.latY, p.lonX, p.latY);

  // (positive XTD means right of course, negative means left)
  double crossTrack = asin(sin(distFrom1) * sin(courseFrom1 - course1To2));

  // The "along track distance", ATD, the distance from A along the course towards B to the point abeam D is given by:
  double distAlongFrom1 = acos(cos(distFrom1) / cos(crossTrack));
  double distAlongFrom2 = acos(cos(distFrom2) / cos(-crossTrack));

  if(!std::isnan(distAlongFrom1) && distAlongFrom1 <= dist1To2 &&
     !std::isnan(distAlongFrom2) && distAlongFrom2 <= dist1To2)
  {
    result.status = ALONG_TRACK;
    result.distance = static_cast<float>(crossTrack * EARTH_RADIUS_METER_DOUBLE);
    result.distanceFrom1 = static_cast<float>(distAlongFrom1 * EARTH_RADIUS_METER_DOUBLE);
    result.distanceFrom2 = static_cast<float>(distAlongFrom2 * EARTH_RADIUS_METER_DOUBLE);
  }
  else if(!std::isnan(distAlongFrom1) && !std::isnan(distAlongFrom2))
  {
    result.status = distFrom1 < distFrom2 ? BEFORE_START : AFTER_END;
    result.distance = static_cast<float>((distFrom1 < distFrom2 ? distFrom1 : distFrom2) * EARTH_RADIUS_METER_DOUBLE);
    result.distanceFrom1 = static_cast<float>(distAlongFrom1 * EARTH_RADIUS_METER_DOUBLE);
    result.distanceFrom2 = static_cast<float>(distAlongFrom2 * EARTH_RADIUS_METER_DOUBLE);
  }
  // else invalid
}

float Pos::angleDegTo(const Pos& otherPos) const
{
  if(!isValid() || !otherPos.isValid())
    return INVALID_VALUE;
  else if(*this == otherPos)
    return INVALID_VALUE;

  double angleDeg = toDegree(courseRad(toRadians(static_cast<double>(lonX)), toRadians(static_cast<double>(latY)),
                                       toRadians(static_cast<double>(otherPos.lonX)), toRadians(static_cast<double>(otherPos.latY))));
  return static_cast<float>(normalizeCourse(angleDeg));
}

float Pos::initialBearing(const Pos& otherPos) const
{
  if(!this->isValid() || !otherPos.isValid())
    return INVALID_FLOAT;

  double delta = otherPos.getLonXRad() - getLonXRad();
  double bearing = atan2(sin(delta) * cos(otherPos.getLatYRad()),
                         cos(getLatYRad()) * sin(otherPos.getLatYRad()) - sin(getLatYRad()) * cos(otherPos.getLatYRad()) * cos(delta));
  return static_cast<float>(normalizeCourse(toDegree(bearing)));
}

float Pos::finalBearing(const Pos& otherPos) const
{
  if(!this->isValid() || !otherPos.isValid())
    return INVALID_FLOAT;

  return normalizeCourse(180.0f + otherPos.initialBearing(*this));
}

Pos Pos::endpointRhumb(float distanceMeter, float angleDeg) const
{
  if(!isValid())
    return EMPTY_POS;

  if(distanceMeter == 0.f)
    return *this;

  double lon1 = toRadians(lonX);
  double lat1 = toRadians(latY);

  double distanceRad = nmToRad(meterToNm(distanceMeter));
  double tc = toRadians(-angleDeg + 360.);

  double lat = lat1 + distanceRad * cos(tc);
  if(std::abs(lat) > M_PI / 2.)
    return atools::geo::Pos(); // distance too long - return invalid pos

  double q, dphi;
  if(atools::almostEqual(lat, lat1))
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
  if(!isValid() || !otherPos.isValid())
    return INVALID_VALUE;
  else if(*this == otherPos)
    return 0.f;

  double lon1 = toRadians(lonX);
  double lat1 = toRadians(latY);
  double lon2 = toRadians(otherPos.lonX);
  double lat2 = toRadians(otherPos.latY);

  double dlonWest = remainder(lon2 - lon1, 2. * M_PI);
  double dlonEast = remainder(lon1 - lon2, 2. * M_PI);
  double dphi = log(tan(lat2 / 2. + M_PI / 4.) / tan(lat1 / 2. + M_PI / 4.));

  double tc;
  if(dlonWest < dlonEast)
    // To west is shorter
    tc = remainder(atan2(-dlonWest, dphi), 2. * M_PI);
  else
    tc = remainder(atan2(dlonEast, dphi), 2. * M_PI);
  return static_cast<float>(normalizeCourse(-toDegree(tc) + 360.));
}

float Pos::distanceMeterToRhumb(const Pos& otherPos) const
{
  if(!isValid() || !otherPos.isValid())
    return INVALID_VALUE;
  else if(*this == otherPos)
    return 0.f;

  double lon1 = toRadians(lonX);
  double lat1 = toRadians(latY);
  double lon2 = toRadians(otherPos.lonX);
  double lat2 = toRadians(otherPos.latY);

  double dlonWest = remainder(lon2 - lon1, 2. * M_PI);
  double dlonEast = remainder(lon1 - lon2, 2. * M_PI);
  double q, distance;
  double dphi = log(tan(lat2 / 2. + M_PI / 4.) / tan(lat1 / 2. + M_PI / 4.));

  if(atools::almostEqual(lat2, lat1))
    q = cos(lat1);
  else
    q = (lat2 - lat1) / dphi;

  if(dlonWest < dlonEast)
    // To west is shorter
    distance = sqrt(q * q * dlonWest * dlonWest + (lat2 - lat1) * (lat2 - lat1));
  else
    distance = sqrt(q * q * dlonEast * dlonEast + (lat2 - lat1) * (lat2 - lat1));

  return static_cast<float>(distance * EARTH_RADIUS_METER_DOUBLE);
}

Pos Pos::interpolateRhumb(const atools::geo::Pos& otherPos, float distanceMeter, float fraction) const
{
  if(!isValid() || !otherPos.isValid())
    return EMPTY_POS;

  return endpointRhumb(distanceMeter * fraction, angleDegToRhumb(otherPos));
}

Pos Pos::interpolateRhumb(const atools::geo::Pos& otherPos, float fraction) const
{
  if(!isValid() || !otherPos.isValid())
    return EMPTY_POS;

  return interpolateRhumb(otherPos, distanceMeterToRhumb(otherPos), fraction);
}

Pos Pos::interpolate(const atools::geo::Pos& otherPos, float fraction) const
{
  if(!isValid() || !otherPos.isValid())
    return EMPTY_POS;

  return interpolate(otherPos, distanceMeterTo(otherPos), fraction);
}

Pos Pos::interpolate(const atools::geo::Pos& otherPos, float distanceMeter, float fraction) const
{
  if(!isValid() || !otherPos.isValid())
    return EMPTY_POS;
  else if(*this == otherPos)
    return *this;

  if(fraction <= 0.f)
    return *this;

  if(fraction >= 1.f)
    return otherPos;

  double lon1 = toRadians(lonX);
  double lat1 = toRadians(latY);
  double lon2 = toRadians(otherPos.lonX);
  double lat2 = toRadians(otherPos.latY);
  double distanceRad = nmToRad(meterToNm(distanceMeter));

  double A = sin((1. - fraction) * distanceRad) / sin(distanceRad);
  double B = sin(fraction * distanceRad) / sin(distanceRad);
  double x = A * cos(lat1) * cos(lon1) + B * cos(lat2) * cos(lon2);
  double y = A * cos(lat1) * sin(lon1) + B * cos(lat2) * sin(lon2);
  double z = A * sin(lat1) + B * sin(lat2);
  double lat = atan2(z, sqrt(x * x + y * y));
  double lon = atan2(y, x);
  return atools::geo::Pos(lon, lat).toDeg().normalize();
}

QString Pos::toLongString(bool starDeg) const
{
  if(!isValid())
    throw Exception("Invalid position. Cannot convert to string");

  return (starDeg ? LONG_FORMAT_STAR : LONG_FORMAT).arg(latY > 0.f ? "N" : "S").
         arg(absInt(getLatYDeg())).arg(absInt(getLatYMin())).arg(std::abs(getLatYSec()), 0, 'f', 2).
         arg(lonX > 0.f ? "E" : "W").
         arg(absInt(getLonXDeg())).arg(absInt(getLonXMin())).arg(std::abs(getLonXSec()), 0, 'f', 2).
         arg(altitude >= 0.f ? "+" : "-").arg(std::abs(altitude), 9, 'f', 2, '0');
}

QString Pos::toHumanReadableString() const
{
  if(!isValid())
    throw Exception("Invalid position. Cannot convert to string");

  return HUMAN_FORMAT.arg(latY > 0.f ? "N" : "S").
         arg(absInt(getLatYDeg())).arg(absInt(getLatYMin())).arg(std::abs(getLatYSec()), 0, 'f', 2).
         arg(lonX > 0.f ? "E" : "W").
         arg(absInt(getLonXDeg())).arg(absInt(getLonXMin())).arg(std::abs(getLonXSec()), 0, 'f', 2);
}

bool Pos::isNull() const
{
  return atools::almostEqual(lonX, 0.f) && atools::almostEqual(latY, 0.f);
}

bool Pos::isNull(float epsilonDegree) const
{
  return atools::almostEqual(lonX, 0.f, epsilonDegree) && atools::almostEqual(latY, 0.f, epsilonDegree);
}

QString Pos::toString(int precision, bool alt) const
{
  if(!isValid())
    return "Invalid Pos";

  if(alt)
    return SHORT_FORMAT_ALT.arg(lonX, 0, 'f', precision).arg(latY, 0, 'f', precision).arg(altitude, 0, 'f', precision);
  else
    return SHORT_FORMAT.arg(lonX, 0, 'f', precision).arg(latY, 0, 'f', precision);
}

void Pos::interpolatePoints(const Pos& otherPos, float distanceMeter, int numPoints, atools::geo::LineString& positions) const
{
  if(!isValid() || !otherPos.isValid())
    return;
  else if(*this == otherPos)
    return;

  float step = 1.f / numPoints;
  for(int j = 0; j < numPoints; j++)
    positions.append(interpolate(otherPos, distanceMeter, step * static_cast<float>(j)).alt(altitude));
}

void Pos::interpolatePointsRhumb(const Pos& otherPos, float distanceMeter, int numPoints, atools::geo::LineString& positions) const
{
  if(!isValid() || !otherPos.isValid())
    return;
  else if(*this == otherPos)
    return;

  float step = 1.f / numPoints;
  for(int j = 0; j < numPoints; j++)
    positions.append(interpolateRhumb(otherPos, distanceMeter, step * static_cast<float>(j)).alt(altitude));
}

void Pos::interpolatePointsAlt(const Pos& otherPos, float distanceMeter, int numPoints,
                               atools::geo::LineString& positions) const
{
  interpolatePoints(otherPos, distanceMeter, numPoints, positions);

  float alt1 = getAltitude(), alt2 = otherPos.getAltitude();
  if(almostNotEqual(alt1, alt2))
  {
    // positions includes this but not pos2 if numPoints > 0
    // interpolate altitude values
    for(Pos& pos : positions)
      // f(x) = f0 + ((f1 - f0) / (x1 - x0)) * (x - x0)
      pos.setAltitude(atools::interpolate(alt1, alt2, 0.f, distanceMeter, distanceMeterTo(pos)));
  }
}

/* Check if seconds or minutes value is rounded up to 60.00 when convertin to string */
inline bool Pos::doesOverflow60(float value) const
{
  // No locale specifics used here
  // String conversion is only option to catch system dependend rounding variances
  return std::abs(value) >= MAX_SECONDS ||
         OVERFLOW_60_TEST.arg(std::abs(value), 0, 'f', 2).startsWith(OVERFLOW_60_TEST_TEXT);
}

int Pos::deg(float value) const
{
  float min = (value - static_cast<int>(value)) * 60.f;
  float seconds = (min - static_cast<int>(min)) * 60.f;

  int degrees = static_cast<int>(value);
  int minutes = static_cast<int>(min);

  // Avoid 60 seconds due to rounding up when converting to text
  if(doesOverflow60(seconds))
    minutes += (minutes > 0 ? 1 : -1);
  if(absInt(minutes) >= 60)
    degrees += (degrees > 0 ? 1 : -1);

  return degrees;
}

int Pos::min(float value) const
{
  float min = (value - static_cast<int>(value)) * 60.f;
  float seconds = (min - static_cast<int>(min)) * 60.f;
  int minutes = static_cast<int>(min);

  // Avoid 60 seconds due to rounding up when converting to text
  if(doesOverflow60(seconds))
    minutes += (minutes > 0 ? 1 : -1);
  if(absInt(minutes) >= 60)
    minutes = 0;

  return minutes;
}

float Pos::sec(float value) const
{
  float min = (value - static_cast<int>(value)) * 60.f;
  float seconds = (min - static_cast<int>(min)) * 60.f;

  // Avoid 60 seconds due to rounding up when converting to text
  if(doesOverflow60(seconds))
    return 0.f;
  else
    return seconds;
}

double Pos::courseRad(double lonX1, double latY1, double lonX2, double latY2)
{
  double val = atan2(sin((lonX2 - lonX1)) * cos((latY2)),
                     cos((latY1)) * sin((latY2)) - sin((latY1)) * cos((latY2)) *
                     cos((lonX2 - lonX1)));

  return std::fmod(val + M_PI * 2., M_PI * 2.);
}

float Pos::meterForDegreeLonx(float latY)
{
  float absLat = std::abs(latY);
  float nmPerDeg;

  if(absLat < 20.f)
    nmPerDeg = 56.f; // at 20°
  else if(absLat < 30.f)
    nmPerDeg = 52.f; // at 30°
  else if(absLat < 40.f)
    nmPerDeg = 46.f; // at 40°
  else if(absLat < 50.f)
    nmPerDeg = 39.f; // at 50°
  else if(absLat < 60.f)
    nmPerDeg = 30.f; // at 60°
  else if(absLat < 65.f)
    nmPerDeg = 25.f; // at 65°
  else if(absLat < 70.f)
    nmPerDeg = 21.f; // at 70°
  else if(absLat < 75.f)
    nmPerDeg = 15.5f; // at 75°
  else if(absLat < 80.f)
    nmPerDeg = 10.4f; // at 80°
  else if(absLat < 85.f)
    nmPerDeg = 5.2f; // at 85°
  else
    nmPerDeg = 1.f; // at 85°

  return atools::geo::nmToMeter(nmPerDeg);
}

double Pos::distanceRad(double lonX1, double latY1, double lonX2, double latY2)
{
  double l1 = (sin(((latY1 - latY2)) / 2.));
  double l2 = (sin(((lonX1 - lonX2)) / 2.));
  return 2. * asin(sqrt(l1 * l1 + cos((latY1)) * cos((latY2)) * l2 * l2));
}

atools::geo::Pos Pos::intersectingRadials(const atools::geo::Pos& p1, float brng1,
                                          const atools::geo::Pos& p2, float brng2)
{
  if(!p1.isValid() || !p2.isValid())
    return EMPTY_POS;
  else if(p1 == p2)
    return p1;

  // double p1 = LatLon(51.8853, 0.2545), brng1 = 108.547;
  // double p2 = LatLon(49.0034, 2.5735), brng2 =  32.435;
  // double pInt = LatLon.intersection(p1, brng1, p2, brng2); // 50.9078°N, 004.5084°E
  double lat1 = atools::geo::toRadians(p1.getLatY()), lon1 = atools::geo::toRadians(p1.getLonX());
  double lat2 = atools::geo::toRadians(p2.getLatY()), lon2 = atools::geo::toRadians(p2.getLonX());
  double brg13 = atools::geo::toRadians(brng1), brg23 = atools::geo::toRadians(brng2);
  double dlat = lat2 - lat1, dlon = lon2 - lon1;

  double dst12 = 2. * asin(sqrt(sin(dlat / 2.) * sin(dlat / 2.) + cos(lat1) * cos(lat2) * sin(dlon / 2.) * sin(dlon / 2.)));
  if(dst12 == 0.)
    return EMPTY_POS;

  // initial/final bearings between points
  double initbrg = acos((sin(lat2) - sin(lat1) * cos(dst12)) / (sin(dst12) * cos(lat1)));
  if(std::isnan(initbrg))
    initbrg = 0.; // protect against rounding
  double finalbrg = acos((sin(lat1) - sin(lat2) * cos(dst12)) / (sin(dst12) * cos(lat2)));

  double crs12 = sin(lon2 - lon1) > 0. ? initbrg : 2. * M_PI - initbrg;
  double crs21 = sin(lon2 - lon1) > 0. ? 2. * M_PI - finalbrg : finalbrg;

  double a1 = remainder(brg13 - crs12 + M_PI, 2. * M_PI) - M_PI; // angle 2-1-3
  double a2 = remainder(crs21 - brg23 + M_PI, 2. * M_PI) - M_PI; // angle 1-2-3

  if(sin(a1) == 0. && sin(a2) == 0.)
    return EMPTY_POS; // infinite intersections

  if(sin(a1) * sin(a2) < 0.)
    return EMPTY_POS; // ambiguous intersection

  double a3 = acos(-cos(a1) * cos(a2) + sin(a1) * sin(a2) * cos(dst12));
  double dist13 = atan2(sin(dst12) * sin(a1) * sin(a2), cos(a2) + cos(a1) * cos(a3));
  double lat3 = asin(sin(lat1) * cos(dist13) + cos(lat1) * sin(dist13) * cos(brg13));
  double dlon13 = atan2(sin(brg13) * sin(dist13) * cos(lat1), cos(dist13) - sin(lat1) * sin(lat3));
  double lon3 = lon1 + dlon13;

  if(std::isnan(lon3) || std::isnan(lat3))
    return EMPTY_POS;
  else
    return Pos(lon3, lat3).toDeg().normalize();
}

atools::geo::Point3D Pos::toCartesian() const
{
  Point3D vector;
  toCartesian(vector);
  return vector;
}

void Pos::toCartesian(atools::geo::Point3D& point) const
{
  float x, y, z;
  toCartesian(x, y, z);
  point.set(x, y, z);
}

void Pos::toCartesian(float& x, float& y, float& z) const
{
  double xx, yy, zz;
  toCartesian(xx, yy, zz);
  x = static_cast<float>(xx);
  y = static_cast<float>(yy);
  z = static_cast<float>(zz);
}

void Pos::toCartesian(double& x, double& y, double& z) const
{
  if(isValid())
  {
    // ISO convention as used in physics
    // Θ, theta, polar angle, latY, 0° to 180°
    // φ, phi, azimuthal angle, lonX, −180° bis 180°
    double theta = static_cast<double>(90.f - latY);
    double phi = static_cast<double>(lonX);

    double sinTheta = sinDeg(theta);
    x = EARTH_RADIUS_METER_DOUBLE * sinTheta * cosDeg(phi);
    y = EARTH_RADIUS_METER_DOUBLE * sinTheta * sinDeg(phi);
    z = EARTH_RADIUS_METER_DOUBLE * cosDeg(theta);
  }
  else
    x = y = z = 0.;
}

double PosD::distanceMeterTo(const PosD& otherPos) const
{
  if(!isValid() || !otherPos.isValid())
    return INVALID_VALUE;
  else if(this->almostEqual(otherPos))
    return 0.;
  else
    return Pos::distanceRad(toRadians(lonX), toRadians(latY),
                            toRadians(otherPos.lonX), toRadians(otherPos.latY)) * Pos::EARTH_RADIUS_METER_DOUBLE;
}

double PosD::angleDegTo(const PosD& otherPos) const
{
  if(!isValid() || !otherPos.isValid())
    return INVALID_VALUE;
  else if(*this == otherPos)
    return INVALID_VALUE;

  return normalizeCourse(toDegree(Pos::courseRad(toRadians(lonX), toRadians(latY), toRadians(otherPos.lonX), toRadians(otherPos.latY))));
}

PosD& PosD::normalize()
{
  if(isValid())
  {
    lonX = normalizeLonXDeg(lonX);
    latY = normalizeLatYDeg(latY);
  }
  return *this;
}

PosD PosD::endpoint(double distanceMeter, double angleDeg) const
{
  if(!isValid())
    return EMPTY_POSD;

  if(distanceMeter == 0.)
    return *this;

  double lon, lat;
  endpointRad(toRadians(lonX), toRadians(latY), meterToRad(distanceMeter), toRadians(-angleDeg + 360.), lon, lat);

  return PosD(toDegree(lon), toDegree(lat)).normalize();
}

bool PosD::almostEqual(const PosD& other, double epsilon) const
{
  return atools::almostEqual(lonX, other.lonX, epsilon) && atools::almostEqual(latY, other.latY, epsilon);
}

bool PosD::isNull() const
{
  return atools::almostEqual(lonX, 0.) && atools::almostEqual(latY, 0.);
}

QDebug operator<<(QDebug out, const Pos& pos)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "Pos(" << pos.toString() << ")";
  return out;
}

QDebug operator<<(QDebug out, const PosD& pos)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "DPos(" << pos.lonX << "," << pos.latY << ")";
  return out;
}

QDataStream& operator<<(QDataStream& out, const Pos& obj)
{
  out << obj.lonX << obj.latY << obj.altitude;
  return out;
}

QDataStream& operator>>(QDataStream& in, Pos& obj)
{
  in >> obj.lonX >> obj.latY >> obj.altitude;
  return in;
}

QDataStream& operator<<(QDataStream& out, const PosD& obj)
{
  out << obj.lonX << obj.latY << obj.altitude;
  return out;
}

QDataStream& operator>>(QDataStream& in, PosD& obj)
{
  in >> obj.lonX >> obj.latY >> obj.altitude;
  return in;
}

QDebug operator<<(QDebug out, const LineDistance& lineDist)
{
  QDebugStateSaver saver(out);
  out << "LineDistance[distance" << atools::geo::meterToNm(lineDist.distance)
      << ", distanceFrom1" << atools::geo::meterToNm(lineDist.distanceFrom1)
      << ", distanceFrom2" << atools::geo::meterToNm(lineDist.distanceFrom2);

  switch(lineDist.status)
  {
    case atools::geo::INVALID:
      out << " INVALID";
      break;
    case atools::geo::ALONG_TRACK:
      out << " ALONG_TRACK";
      break;
    case atools::geo::BEFORE_START:
      out << " BEFORE_START";
      break;
    case atools::geo::AFTER_END:
      out << " AFTER_END";
      break;
  }
  out << "]";
  return out;
}

} // namespace geo
} // namespace atools
