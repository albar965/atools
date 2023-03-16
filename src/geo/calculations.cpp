/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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
#include "geo/line.h"
#include "geo/linestring.h"

#include <QDateTime>
#include <QLineF>
#include <QPointF>
#include <QRect>

namespace atools {

namespace geo {

float distanceToLine(float x, float y, float x1, float y1, float x2, float y2, bool lineOnly,
                     LineDist *distType)
{
  float vx = x2 - x1, vy = y2 - y1;
  float wx = x - x1, wy = y - y1;
  float dist = INVALID_FLOAT;
  float c1 = vx * wx + vy * wy; // dot product
  if(c1 <= 0)
  {
    if(!lineOnly)
      dist = atools::geo::simpleDistanceF(x, y, x1, y1);
    if(distType != nullptr)
      *distType = DIST_TO_START;
  }
  else
  {
    float c2 = vx * vx + vy * vy; // dot product
    if(c2 <= c1)
    {
      if(!lineOnly)
        dist = atools::geo::simpleDistanceF(x, y, x2, y2);
      if(distType != nullptr)
        *distType = DIST_TO_END;
    }
    else
    {
      // get line in parameter form
      float a = y1 - y2, b = x2 - x1, c = x1 * y2 - x2 * y1;
      dist = std::abs(a * x + b * y + c) / std::sqrt(a * a + b * b);
      if(distType != nullptr)
        *distType = DIST_TO_LINE;
    }
  }
  return dist;
}

void arcFromPoints(const QLineF& line, const QPointF& center, bool left, QRectF *rect, float *startAngle,
                   float *spanAngle)
{
  // center of the circle is (x0, y0) and that the arc contains your two points (x1, y1) and (x2, y2).
  // Then the radius is: r=sqrt((x1-x0)(x1-x0) + (y1-y0)(y1-y0)).
  double radius = QLineF(center, line.p1()).length();

  if(rect != nullptr)
    *rect = QRectF(center.x() - radius, center.y() - radius, 2. * radius, 2. * radius);

  if(startAngle != nullptr || spanAngle != nullptr)
  {
    double start = normalizeCourse(toDegree(std::atan2(line.y1() - center.y(), line.x1() - center.x())));
    double end = normalizeCourse(toDegree(std::atan2(line.y2() - center.y(), line.x2() - center.x())));

    double span = 0.;
    if(left)
    {
      if(start > end)
        span = start - end;
      else
        span = 360. - end + start;
    }
    else
    {
      // negative values mean clockwise
      if(end > start)
        span = end - start;
      else
        span = 360. - start + end;
      span = -span;
    }

    if(startAngle != nullptr)
      *startAngle = static_cast<float>(start);

    if(spanAngle != nullptr)
      *spanAngle = static_cast<float>(span);
  }
}

void calcArcLength(const atools::geo::Line& line, const atools::geo::Pos& center, bool left,
                   float *distance, atools::geo::LineString *lines)
{
  if(distance != nullptr)
    *distance = 0.f;

  if(line.getPos1().almostEqual(line.getPos2()))
  {
    if(lines != nullptr)
      lines->append(line.getPos1());
    return;
  }

  float dist = center.distanceMeterTo(line.getPos1());
  float start = center.angleDegTo(line.getPos1());
  float end = center.angleDegTo(line.getPos2());
  float spanningAngle;

  if(left)
  {
    if(start < end)
      // left = CCW 10 - 340
      spanningAngle = start + (360.f - end);
    else
      // left = CCW 260 - 100
      spanningAngle = start - end;
  }
  else
  {
    if(start < end)
      // right = CW 90 - 270
      spanningAngle = (end - start);
    else
      // right = CW 340 - 10
      spanningAngle = ((360.f - start) + end);
  }

  // Calculate number of steps for 10 degrees
  int numSteps = std::max(static_cast<int>(spanningAngle / 10.f), 1);

  float step = spanningAngle / static_cast<float>(numSteps);

  Pos last = line.getPos1();
  for(int i = 0; i <= numSteps; i++)
  {
    float angle;

    if(left)
      angle = start - static_cast<float>(i) * step;
    else
      angle = start + static_cast<float>(i) * step;

    angle = normalizeCourse(angle);
    Pos cur = center.endpoint(dist, angle);
    if(lines != nullptr)
      lines->append(cur);

    if(distance != nullptr)
      *distance += last.distanceMeterTo(cur);
    last = cur;
  }
}

Rect boundingRect(const QList<Pos>& positions)
{
  Rect rect;
  boundingRect(rect, positions);
  return rect;
}

void boundingRect(Rect& rect, QList<atools::geo::Pos> positions)
{
  // Remove all invalid positions
  auto iter = std::remove_if(positions.begin(), positions.end(), [](const atools::geo::Pos& p) -> bool
      {
        return !p.isValid();
      });

  if(iter != positions.end())
    positions.erase(iter, positions.end());

  // If the line string is empty return an empty boundingbox
  if(positions.size() == 0)
  {
    rect = Rect();
    return;
  }

  float lonX = positions.constBegin()->getLonX(), latY = positions.constBegin()->getLatY();

  float north = latY;
  float south = latY;
  float west = lonX;
  float east = lonX;

  // If there's only a single node stored then the boundingbox only contains that point
  if(positions.size() == 1)
  {
    rect = Rect(west, north, east, south);
    return;
  }

  // Specifies whether the polygon crosses the IDL
  bool idlCrossed = false;

  // "idlCrossState" specifies the state concerning IDL crossage.
  // This is needed in order to create optimal bounding boxes in case of covering the IDL
  // Every time the IDL gets crossed from east to west the idlCrossState value gets
  // increased by one.
  // Every time the IDL gets crossed from west to east the idlCrossState value gets
  // decreased by one.

  int idlCrossState = 0;
  int idlMaxCrossState = 0;
  int idlMinCrossState = 0;

  // Holds values for east and west while idlCrossState != 0
  float otherWest = lonX;
  float otherEast = lonX;

  float previousLon = lonX;

  int currentSign = (lonX < 0) ? -1 : +1;
  int previousSign = currentSign;

  auto it(positions.constBegin());
  auto itEnd(positions.constEnd());

  bool processingLastNode = false;

  while(it != itEnd)
  {
    // Get coordinates and normalize them to the desired range.
    lonX = it->getLonX();
    latY = it->getLatY();

    // Determining the maximum and minimum latitude
    if(latY > north)
      north = latY;
    if(latY < south)
      south = latY;

    currentSign = (lonX < 0.f) ? -1 : +1;

    // Once the polyline crosses the dateline the covered bounding box
    // would cover the whole [-M_PI; M_PI] range.
    // When looking separately at the longitude range that gets covered
    // east and west from the IDL we get two bounding boxes (we prefix
    // the resulting longitude range on the "other side" with "other").
    // By picking the "inner" range values we get a more appropriate
    // optimized single bounding box.

    // IDL check
    if(previousSign != currentSign && fabs(previousLon) + fabs(lonX) > 180.f)
    {

      // Initialize values for otherWest and otherEast
      if(idlCrossed == false)
      {
        otherWest = lonX;
        otherEast = lonX;
        idlCrossed = true;
      }

      // Determine the new IDL Cross State
      if(previousLon < 0.f)
      {
        idlCrossState++;
        if(idlCrossState > idlMaxCrossState)
          idlMaxCrossState = idlCrossState;
      }
      else
      {
        idlCrossState--;
        if(idlCrossState < idlMinCrossState)
          idlMinCrossState = idlCrossState;
      }
    }

    if(idlCrossState == 0)
    {
      if(lonX > east)
        east = lonX;
      if(lonX < west)
        west = lonX;
    }
    else
    {
      if(lonX > otherEast)
        otherEast = lonX;
      if(lonX < otherWest)
        otherWest = lonX;
    }

    previousLon = lonX;
    previousSign = currentSign;

    if(processingLastNode)
      break;

    ++it;

    if(positions.constFirst() == positions.constLast() /* is closed */ && it == itEnd)
    {
      it = positions.constBegin();
      processingLastNode = true;
    }
  }

  if(idlCrossed)
  {
    if(idlMinCrossState < 0)
      east = otherEast;
    if(idlMaxCrossState > 0)
      west = otherWest;

    if((idlMinCrossState < 0 && idlMaxCrossState > 0) ||
       idlMinCrossState < -1 || idlMaxCrossState > 1 ||
       west <= east)
    {
      east = +180.f;
      west = -180.f;
      // if polygon fully in south hemisphere, contain south pole
      if(north < 0)
        south = -90.f;
      else
        north = 90.f;
    }
  }

  // explicit Rect(float leftLonX, float topLatY, float rightLonX, float bottomLatY);
  // return Rect(north, south, east, west);
  rect = Rect(west, north, east, south);
}

QRect rectToSquare(const QRect& rect)
{
  QRect retval = rect.normalized();
  if(retval.width() > retval.height())
    retval.setRect(retval.x(), retval.y() - (retval.width() - retval.height()) / 2,
                   retval.width(), retval.width());
  else if(retval.width() < retval.height())
    retval.setRect(retval.x() - (retval.height() - retval.width()) / 2, retval.y(),
                   retval.height(), retval.height());
  return retval;
}

QRectF rectToSquare(const QRectF& rect)
{
  QRectF retval = rect.normalized();
  if(retval.width() > retval.height())
    retval.setRect(retval.x(), retval.y() - (retval.width() - retval.height()) / 2.,
                   retval.width(), retval.width());
  else if(retval.width() < retval.height())
    retval.setRect(retval.x() - (retval.height() - retval.width()) / 2., retval.y(),
                   retval.height(), retval.height());
  return retval;
}

QTime calculateSunriseSunset(bool& neverRises, bool& neverSets, const atools::geo::Pos& position, const QDate& date,
                             float zenith)
{
  neverRises = neverSets = false;

  // 1. first calculate the day of the year
  int dayOfYear = date.dayOfYear();

  // 2. convert the longitude to hour value and calculate an approximate time
  double longitudeHour = position.getLonX() / 15.;
  double t;
  if(zenith > 0.)
    t = dayOfYear + ((6. - longitudeHour) / 24.);
  else
    t = dayOfYear + ((18. - longitudeHour) / 24.);

  // 3. calculate the Sun's mean anomaly
  double sunMeanAnomaly = (0.9856 * t) - 3.289;

  // 4. calculate the Sun's true longitude
  double sunLongitude = sunMeanAnomaly +
                        (1.916 * sinDeg(sunMeanAnomaly)) +
                        (0.020 * sinDeg(2. * sunMeanAnomaly)) + 282.634;
  if(sunLongitude > 360.)
    sunLongitude -= 360.;
  if(sunLongitude < 0.)
    sunLongitude += 360.;

  // 5a. calculate the Sun's right ascension
  double rightAcension = atanDeg(0.91764 * tanDeg(sunLongitude));

  // 5b. right ascension value needs to be in the same quadrant as L
  double longitudeQuadrant = (floor(sunLongitude / 90.)) * 90.;
  double raQuadrant = (floor(rightAcension / 90.)) * 90.;
  rightAcension = rightAcension + (longitudeQuadrant - raQuadrant);

  // 5c. right ascension value needs to be converted into hours
  rightAcension = rightAcension / 15.;

  // 6. calculate the Sun's declination
  double sinDeclination = 0.39782 * sinDeg(sunLongitude);
  double cosDeclination = cosDeg(asinDeg(sinDeclination));

  // 7a. calculate the Sun's local hour angle
  double cosHourAngle = (cosDeg(std::abs(zenith)) - (sinDeclination * sinDeg(position.getLatY()))) /
                        (cosDeclination * cosDeg(position.getLatY()));

  if(cosHourAngle > 1.)
  {
    // the sun never rises on this location (on the specified date)
    neverRises = true;
    return QTime();
  }

  if(cosHourAngle < -1.)
  {
    // the sun never sets on this location (on the specified date)
    neverSets = true;
    return QTime();
  }

  // 7b. finish calculating H and convert into hours
  double hourAngle;
  if(zenith > 0.)
    hourAngle = 360. - acosDeg(cosHourAngle);
  else
    hourAngle = acosDeg(cosHourAngle);

  // H = H / 15
  hourAngle = hourAngle / 15.;

  // 8. calculate local mean time of rising/setting
  double localMeanTime = hourAngle + rightAcension - (0.06571 * t) - 6.622;

  // 9. adjust back to UTC
  double utcTime = localMeanTime - longitudeHour;

  if(utcTime > 24.)
    utcTime -= 24.;
  if(utcTime < 0.)
    utcTime += 24.;

  // Erase milliseconds
  QTime time = QTime::fromMSecsSinceStartOfDay(atools::roundToInt(utcTime * 3600) * 1000);
  return time;
}

double windCorrectedHeadingRad(double& groundSpeed, double windSpeed, double windDirectionRad, double courseRad,
                               double trueAirspeed)
{
  if(almostEqual(windSpeed, 0.) && trueAirspeed > 0.)
  {
    // No wind - course = heading, GS = TAS
    groundSpeed = trueAirspeed;
    return courseRad;
  }

  if(almostEqual(trueAirspeed, 0.))
  {
    groundSpeed = trueAirspeed;

    // No airspeed - heading invalid
    return INVALID_DOUBLE;
  }

  double swc = (windSpeed / trueAirspeed) * sin(windDirectionRad - courseRad);

  double correctedHeading = 0.;
  if(std::abs(swc) >= 1.)
    // course cannot be flown-- wind too strong
    return INVALID_DOUBLE;
  else
  {
    correctedHeading = courseRad + asin(swc);

    // Correct range
    if(correctedHeading < 0.)
      correctedHeading = correctedHeading + 2. * M_PI;
    if(correctedHeading > 2. * M_PI)
      correctedHeading = correctedHeading - 2. * M_PI;

    groundSpeed = trueAirspeed * sqrt(1. - (swc * swc)) - windSpeed * cos(windDirectionRad - courseRad);
    if(groundSpeed <= 0.)
    {
      groundSpeed = INVALID_DOUBLE;
      // course cannot be flown-- wind too strong
      return INVALID_DOUBLE;
    }
  }

  return correctedHeading;
}

float windCorrectedHeading(float windSpeed, float windDirectionDeg, float courseDeg, float trueAirspeed)
{
  float gsDummy;
  return windCorrectedHeading(gsDummy, windSpeed, windDirectionDeg, courseDeg, trueAirspeed);
}

float windCorrectedGroundSpeed(float windSpeed, float windDirectionDeg, float courseDeg, float trueAirspeed)
{
  float gs;
  windCorrectedHeading(gs, windSpeed, windDirectionDeg, courseDeg, trueAirspeed);
  return gs;
}

float windCorrectedHeading(float& groundSpeed, float windSpeed, float windDirectionDeg, float courseDeg,
                           float trueAirspeed)
{
  if(!(courseDeg < INVALID_FLOAT / 2.f) ||
     !(windDirectionDeg < INVALID_FLOAT / 2.f))
    return INVALID_FLOAT;

  double gs;
  double heading = windCorrectedHeadingRad(gs, windSpeed, toRadians(windDirectionDeg), toRadians(
                                             courseDeg), trueAirspeed);

  if(gs < INVALID_DOUBLE && gs > 1.f)
    groundSpeed = static_cast<float>(gs);
  else
    groundSpeed = INVALID_FLOAT;

  if(heading < INVALID_DOUBLE / 2.)
    return static_cast<float>(normalizeCourse(toDegree(heading)));
  else
    return INVALID_FLOAT;
}

// Unit conversions
const static float FEET2METER = 0.3048f;
const static float FEET2NM = 1.64578833693305e-04f;
const static float NM2METER = 1852.f;
const static float FEETPERSEC2KT = FEET2NM * 3600.f;
const static float METERPERSEC2KT = 3600.f / NM2METER;

// Pressure
const static float p0 = 101325.f; // Pascal at MSL
const static float p1 = 22632.05545875171f; // Pascal at 11000 m
const static float p2 = 5474.884659730908f; // Pascal at 20000 m
const static float p3 = 868.0176477556424f; // Pascal at 32000 m

// Temperature
const static float T0 = 288.15f; // Kelvin at MSL
const static float T1 = 216.65f; // Kelvin 11000 - 20000 m
const static float T2 = 216.65f; // Kelvin 20000 - 32000 m

// Altitude layers
const static float alt1Ft = 36089.238845144355f; // ft = 11000 m
const static float alt2Ft = 65616.79790026246f; // ft = 20000 m
const static float alt3Ft = 104986.87664041994f; // ft = 32000 m

// Temperature gradient
const static float deltaTdh0 = -0.0019812f; // Kelvin/ft MSL - 11000 m
const static float deltaTdh0SI = -0.0065f; // Kelvin/m MSL - 11000 m
const static float deltaTdh2 = 0.0003048f; // Kelvin/ft 11000 - 32000 m
const static float deltaTdh2SI = 0.001f; // Kelvin/m 11000 - 32000 m

// Other constants
const static float RGasSI = 287.053f; // J/(kg*Kelvin) Gas constant for for dry air
const static float gSI = 9.80665f; // acceleration on earth m/s^2
const static float gRGasSI = gSI / RGasSI;
const static float gRGas = (gSI * FEET2METER) / RGasSI;
const static float gamma = 1.4f; // -
const static float gammaRGas = (gamma * RGasSI) / (FEET2METER * FEET2METER); // ft^2/(s^2*Kelvin)
const static float aSLSI = std::sqrt(gamma * RGasSI * T0);
const static float pressureSLSI = p0; // Pascal
const static float aSLNU = aSLSI * METERPERSEC2KT; // kts

bool isaAtmosphere(float& temperatureK, float& pressurePa, float altitudeFt)
{
  temperatureK = 0.f;
  pressurePa = 0.f;

  if(altitudeFt <= alt1Ft)
  {
    // Troposphere
    temperatureK = T0 + deltaTdh0 * altitudeFt;
    pressurePa = p0 * std::pow((T0 / temperatureK), (gRGasSI / deltaTdh0SI));
  }
  else if(altitudeFt <= alt2Ft)
  {
    // Tropopause
    temperatureK = T1;
    pressurePa = p1 * std::exp((gRGas / T1) * (alt1Ft - altitudeFt));
  }
  else if(altitudeFt <= alt3Ft)
  {
    // Stratosphere
    temperatureK = T2 + deltaTdh2 * (altitudeFt - alt2Ft);
    pressurePa = p2 * std::pow((T2 / temperatureK), (gRGasSI / deltaTdh2SI));
  }
  else
  {
    // Not valid - too hight
    temperatureK = 0.f;
    pressurePa = 0.f;
    return false;
  }

  return true;
}

float machCrossover(float casKts, float mach)
{
  float p =
    (p0 *
     ((std::pow(((std::pow(casKts, 2.f) / (5.f * std::pow(aSLNU, 2.f))) + 1.f),
                (gamma / (gamma - 1.f)))) - 1.f)) /
    ((std::pow((((std::pow(mach, 2.f)) / 5.f) + 1.f), (gamma / (gamma - 1.f)))) - 1.f);
  float altFt;

  if(p < p1)
    altFt = alt1Ft - (T1 / gRGas) * std::log(p / p1);
  else
    altFt = (((T0 * (std::pow((p / p0), (-deltaTdh0SI / gRGasSI)))) - T0) / deltaTdh0);

  return altFt;
}

bool speedFromCAS(float& mach, float& tasKts, float casKts, float altFt, float isaDelta)
{
  float T = 0.0;
  float p = 0.0;

  if(!isaAtmosphere(T, p, altFt))
    return false;

  T = T + isaDelta;

  float a = std::sqrt(gammaRGas * T);

  tasKts = std::sqrt(5.f) * a *
           std::sqrt(std::pow(((pressureSLSI / p) *
                               (std::pow((casKts * casKts / (5.0f * aSLNU * aSLNU)) + 1.f,
                                         (gamma / (gamma - 1.f))) - 1.f) + 1.f), (gamma - 1.f) / gamma) - 1.f);

  mach = tasKts / a;

  tasKts *= FEETPERSEC2KT;
  return true;
}

bool speedFromMach(float& casKts, float& tasKts, float mach, float altFt, float isaDelta)
{
  float T, p;
  if(!isaAtmosphere(T, p, altFt))
    return false;

  T += isaDelta;

  float a = std::sqrt(gammaRGas * T);

  tasKts = mach * a;
  tasKts *= FEETPERSEC2KT;

  casKts = std::sqrt(5.f) * aSLNU *
           std::sqrt(std::pow(((p / pressureSLSI) *
                               (std::pow((tasKts * tasKts / (5.0f * a * a)) + 1.f,
                                         (gamma / (gamma - 1.f))) - 1.f) + 1.f),
                              (gamma - 1.f) / gamma) - 1.f);

  return true;
}

bool speedFromTAS(float& casKts, float& mach, float tasKts, float altFt, float isaDelta)
{
  tasKts /= FEETPERSEC2KT;

  float T, p;
  if(!isaAtmosphere(T, p, altFt))
    return false;

  T += isaDelta;

  float a = std::sqrt(gammaRGas * T);

  mach = tasKts / a;

  casKts = std::sqrt(5.f) * aSLNU *
           std::sqrt(std::pow(((p / pressureSLSI) *
                               (std::pow((tasKts * tasKts / (5.f * a * a)) + 1.f,
                                         (gamma / (gamma - 1.f))) - 1.f) + 1.f),
                              (gamma - 1.f) / gamma) - 1.f);

  return true;
}

void windForCourse(float& headWind, float& crossWind, float windSpeed, float windDirectionDeg, float courseDeg)
{
  float diffRad = atools::geo::toRadians(windDirectionDeg - courseDeg);
  headWind = windSpeed * std::cos(diffRad);
  crossWind = windSpeed * std::sin(diffRad);
}

float headWindForCourse(float windSpeed, float windDirectionDeg, float courseDeg)
{
  return windSpeed * std::cos(atools::geo::toRadians(windDirectionDeg - courseDeg));
}

bool isJetFuel(float fuelWeightLbs, float fuelQuantityGal, float& weightVolRatio)
{
  if(fuelWeightLbs > 5.f && fuelQuantityGal > 1.f)
  {
    weightVolRatio = fuelWeightLbs / fuelQuantityGal;

    if(atools::almostEqual(weightVolRatio, 6.f, 0.2f))
      return false;
    else if(atools::almostEqual(weightVolRatio, 6.7f, 0.3f))
      return true;
  }
  weightVolRatio = 0.f;
  return false;
}

bool crossesAntiMeridian(float lonx1, float lonx2)
{
  // No crossing if any point is on the anti-meridian
  if(atools::almostEqual(lonx1, 180.f) || atools::almostEqual(lonx1, -180.f) ||
     atools::almostEqual(lonx2, 180.f) || atools::almostEqual(lonx2, -180.f))
    return false;

  // if the absolute value of the difference of longitudes is 180 or greater, there is a crossing.
  // east / west
  return std::abs((lonx1 + 360.f) - (lonx2 + 360.f)) > 180.f ||
         (atools::almostEqual(lonx1, 180.f) && atools::almostEqual(lonx2, -180.f)) ||
         (atools::almostEqual(lonx2, 180.f) && atools::almostEqual(lonx1, -180.f));
}

void registerMetaTypes()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  qRegisterMetaTypeStreamOperators<atools::geo::Pos>();
  qRegisterMetaTypeStreamOperators<atools::geo::Rect>();
  qRegisterMetaTypeStreamOperators<atools::geo::Line>();
  qRegisterMetaTypeStreamOperators<atools::geo::LineString>();
#endif
}

bool isWestCourse(float lonx1, float lonx2)
{
  // Fix all the corner cases around the anti-meridian
  if((almostEqual(lonx2, 180.f) && lonx1 < 0.f) || (almostEqual(lonx2, -180.f) && lonx1 > 0.f))
    lonx2 = -lonx2;

  if((almostEqual(lonx1, 180.f) && lonx2 < 0.f) || (almostEqual(lonx1, -180.f) && lonx2 > 0.f))
    lonx1 = -lonx1;

  // Either equal or -90/90 or 90/-90 where result is undefined
  if(almostEqual(lonx1, lonx2) || almostEqual(angleAbsDiff(lonx1, lonx2), 180.f))
    return false;

  if(crossesAntiMeridian(lonx1, lonx2))
    return lonx2 < 0.f ? (lonx1 > lonx2 + 360.f) : (lonx1 + 360.f > lonx2);
  else
    return lonx1 > lonx2;
}

bool isEastCourse(float lonx1, float lonx2)
{
  // Fix all the corner cases around the anti-meridian
  if((almostEqual(lonx2, 180.f) && lonx1 < 0.f) || (almostEqual(lonx2, -180.f) && lonx1 > 0.f))
    lonx2 = -lonx2;

  if((almostEqual(lonx1, 180.f) && lonx2 < 0.f) || (almostEqual(lonx1, -180.f) && lonx2 > 0.f))
    lonx1 = -lonx1;

  // Either equal or -90/90 or 90/-90 where result is undefined
  if(almostEqual(lonx1, lonx2) || almostEqual(angleAbsDiff(lonx1, lonx2), 180.f))
    return false;

  if(crossesAntiMeridian(lonx1, lonx2))
    return lonx2 < 0.f ? (lonx1 < lonx2 + 360.f) : (lonx1 + 360.f < lonx2);
  else
    return lonx1 < lonx2;
}

} // namespace geo
} // namespace atools
