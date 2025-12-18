// metar interface class
//
// Written by Melchior FRANZ, started December 2003.
//
// Copyright (C) 2003  Melchior FRANZ - mfranz@aon.at
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

/*
 * Interface for encoded Meteorological Aerodrome Reports (METAR).
 *
 * @see WMO-49
 * Technical Regulations, Basic Documents No. 2 (WMO No. 49)
 * Volume II - Meteorological Service for International Air Navigation
 * http://library.wmo.int/pmb_ged/wmo_49-v2_2013_en.pdf
 *
 * Refer to Table A3-2 (Template for METAR and SPECI) following page 78.
 *
 * For general information:
 * World Meteorological Organization http://library.wmo.int
 */
#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "geo/calculations.h"
#include "fs/weather/metarparser.h"

#include <string>
#include <time.h>
#include <cstring>
#include <iostream>

#include <QDateTime>
#include <QStringBuilder>
#include <QTimeZone>
#include <QDebug>

// #define QT_NO_CAST_FROM_BYTEARRAY
// #define QT_NO_CAST_TO_ASCII
// #define QT_NO_CAST_FROM_ASCII

#pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
#pragma GCC diagnostic ignored "-Wcomma"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wimplicit-float-conversion"
#pragma GCC diagnostic ignored "-Wimplicit-int-conversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"

using std::string;
using std::map;
using std::vector;

/** Feet to Meters */
#define SG_FEET_TO_METER 0.3048

/** Meters to Feet */
#define SG_METER_TO_FEET 3.28083989501312335958

/** Meters to Nautical Miles.  1 nm = 6076.11549 feet */
#define SG_METER_TO_NM 0.0005399568034557235

/** Nautical Miles to Meters */
#define SG_NM_TO_METER 1852.0000

/** Meters to Statute Miles. */
#define SG_METER_TO_SM 0.0006213699494949496

/** Statute Miles to Meters. */
#define SG_SM_TO_METER 1609.3412196

/** Radians to Nautical Miles.  1 nm = 1/60 of a degree */
#define SG_NM_TO_RAD 0.00029088820866572159

/** Nautical Miles to Radians */
#define SG_RAD_TO_NM 3437.7467707849392526

/** meters per second to Knots */
#define SG_MPS_TO_KT 1.9438444924406046432

/** Knots to meters per second */
#define SG_KT_TO_MPS 0.5144444444444444444

/** Feet per second to Knots */
#define SG_FPS_TO_KT 0.5924838012958962841

/** Knots to Feet per second */
#define SG_KT_TO_FPS 1.6878098571011956874

/** meters per second to Miles per hour */
#define SG_MPS_TO_MPH 2.2369362920544020312

/** meetrs per hour to Miles per second */
#define SG_MPH_TO_MPS 0.44704

/** Meters per second to Kilometers per hour */
#define SG_MPS_TO_KMH 3.6

/** Kilometers per hour to meters per second */
#define SG_KMH_TO_MPS 0.2777777777777777778

/** Pascal to Inch Mercury */
#define SG_PA_TO_INHG 0.0002952998330101010

/** Inch Mercury to Pascal */
#define SG_INHG_TO_PA 3386.388640341

/** slug/ft3 to kg/m3 */
#define SG_SLUGFT3_TO_KGPM3 515.379

namespace atools {
namespace fs {
namespace weather {

const MetarParser MetarParser::EMPTY;
const QLatin1String MetarParser::NO_IDENT("XXXX");

static QList<Token> description;
static QList<Token> phenomenon;
static QList<Token> special;
static QList<Token> colors;
static QList<Token> cloud_types;

static QStringList runway_deposit;
static QStringList runway_deposit_extent;
static QStringList runway_friction;

void initTranslateableTexts()
{
  description = {
    {"SH", MetarParser::tr("Showers of")},
    {"TS", MetarParser::tr("Thunderstorm with")},
    {"BC", MetarParser::tr("Patches of")},
    {"BL", MetarParser::tr("Blowing")},
    {"DR", MetarParser::tr("Low drifting")},
    {"FZ", MetarParser::tr("Freezing")},
    {"MI", MetarParser::tr("Shallow")},
    {"PR", MetarParser::tr("Partial")},
    {QString(), QString()}
  };

  phenomenon = {
    {"DZ", MetarParser::tr("Drizzle")},
    {"GR", MetarParser::tr("Hail")},
    {"GS", MetarParser::tr("Small hail and/or snow pellets")},
    {"IC", MetarParser::tr("Ice crystals")},
    {"PE", MetarParser::tr("Ice pellets")},
    {"RA", MetarParser::tr("Rain")},
    {"SG", MetarParser::tr("Snow grains")},
    {"SN", MetarParser::tr("Snow")},
    {"UP", MetarParser::tr("Unknown precipitation")},
    {"BR", MetarParser::tr("Mist")},
    {"DU", MetarParser::tr("Widespread dust")},
    {"FG", MetarParser::tr("Fog")},
    {"FGBR", MetarParser::tr("Fog bank")},
    {"FU", MetarParser::tr("Smoke")},
    {"HZ", MetarParser::tr("Haze")},
    {"PY", MetarParser::tr("Spray")},
    {"SA", MetarParser::tr("Sand")},
    {"VA", MetarParser::tr("Volcanic ash")},
    {"DS", MetarParser::tr("Duststorm")},
    {"FC", MetarParser::tr("Funnel cloud/tornado waterspout")},
    {"PO", MetarParser::tr("Well-developed dust/sand whirls")},
    {"SQ", MetarParser::tr("Squalls")},
    {"SS", MetarParser::tr("Sandstorm")},
    {"UP", MetarParser::tr("Unknown")}, // ... due to failed automatic acquisition
    {QString(), QString()}
  };

  special = {
    {"NSW", MetarParser::tr("No significant weather")},
    {QString(), QString()}
    /* { "VCSH", "showers in the vicinity" },
     * { "VCTS", "thunderstorm in the vicinity" }, */
  };

  colors = {
    {"BLU", MetarParser::tr("Blue")}, // 2500 ft,  8.0 km
    {"WHT", MetarParser::tr("White")}, // 1500 ft,  5.0 km
    {"GRN", MetarParser::tr("Green")}, // 700 ft,  3.7 km
    {"YLO", MetarParser::tr("Yellow")}, // 300 ft,  1.6 km
    {"AMB", MetarParser::tr("Amber")}, // 200 ft,  0.8 km
    {"RED", MetarParser::tr("Red")},
    {QString(), QString()}
  };

  cloud_types = {
    {"AC", MetarParser::tr("altocumulus")},
    {"ACC", MetarParser::tr("altocumulus castellanus")},
    {"ACSL", MetarParser::tr("altocumulus standing lenticular")},
    {"AS", MetarParser::tr("altostratus")},
    {"CB", MetarParser::tr("cumulonimbus")},
    {"CBMAM", MetarParser::tr("cumulonimbus mammatus")},
    {"CC", MetarParser::tr("cirrocumulus")},
    {"CCSL", MetarParser::tr("cirrocumulus standing lenticular")},
    {"CI", MetarParser::tr("cirrus")},
    {"CS", MetarParser::tr("cirrostratus")},
    {"CU", MetarParser::tr("cumulus")},
    {"CUFRA", MetarParser::tr("cumulus fractus")},
    {"NS", MetarParser::tr("nimbostratus")},
    {"SAC", MetarParser::tr("stratoaltocumulus")}, // guessed
    {"SC", MetarParser::tr("stratocumulus")},
    {"SCSL", MetarParser::tr("stratocumulus standing lenticular")},
    {"ST", MetarParser::tr("stratus")},
    {"STFRA", MetarParser::tr("stratus fractus")},
    {"TCU", MetarParser::tr("towering cumulus")},
    {QString(), QString()}
  };

  runway_deposit = QStringList({
          MetarParser::tr("clear and dry"),
          MetarParser::tr("damp"),
          MetarParser::tr("wet or puddles"),
          MetarParser::tr("frost"),
          MetarParser::tr("dry snow"),
          MetarParser::tr("wet snow"),
          MetarParser::tr("slush"),
          MetarParser::tr("ice"),
          MetarParser::tr("compacted snow"),
          MetarParser::tr("frozen ridges"),
          QString()
        });

  runway_deposit_extent = QStringList({
          QString(),
          MetarParser::tr("1-10%"),
          MetarParser::tr("11-25%"),
          QString(),
          QString(),
          MetarParser::tr("26-50%"),
          QString(),
          QString(),
          QString(),
          MetarParser::tr("51-100%"),
          QString()
        });

  runway_friction = QStringList({
          QString(),
          MetarParser::tr("poor braking action"),
          MetarParser::tr("poor/medium braking action"),
          MetarParser::tr("medium braking action"),
          MetarParser::tr("medium/good braking action"),
          MetarParser::tr("good braking action"),
          QString(),
          QString(),
          QString(),
          MetarParser::tr("friction: unreliable measurement"),
          QString()
        });
}

void MetarParser::resetParsed()
{
  errors.clear();
  groupCount = 0;
  _year = _month = _day = _hour = _minute = _report_type = -1;

  _wind_dir = _wind_range_from = -1;
  _wind_speed = _gust_speed = INVALID_METAR_VALUE;

  _wind_range_to = -1;
  _temp = _dewp = _pressure = INVALID_METAR_VALUE;

  parsed = _cavok = false;
  _rain = _hail = _snow = 0;

  _icao[0] = '\0';
  _weather2.clear();
  flightRules = UNKNOWN;
  prevailingWindDir = -1;
  prevailingWindSpeed = INVALID_METAR_VALUE;

  _min_visibility = _max_visibility = _vert_visibility = MetarVisibility();
  for(int i = 0; i < 8; i++)
    _dir_visibility[i] = MetarVisibility();

  _clouds.clear();
  _runways.clear();
  _weather.clear();
  _remark.clear();
  unusedData.clear();

  maxCoverageCloud = MetarCloud();
  lowestCoverageCloud = MetarCloud();

  delete[] _data;
  _data = nullptr;
}

const QString MetarParser::getMetarDisplayString() const
{
  if(incompleteInterpolation && metar.startsWith(NO_IDENT % ' '))
    return metar.mid(5);
  else
    return metar;
}

// ===========================
inline bool valid(float value)
{
  return value < INVALID_METAR_VALUE / 2.f;
}

inline bool valid(double value)
{
  return value < INVALID_METAR_VALUE / 2.;
}

inline bool valid(int value)
{
  return value != -1;
}

inline double lambda(double distance)
{
  return distance > 0.000001f ? 1. / distance : 10000000.;
}

/* Merge values using Inverse distance weighting
 * https://de.wikipedia.org/wiki/Inverse_Distanzwichtung (German) */
template<typename TYPE>
TYPE mergeField(const MetarParserList& metars, const QList<float>& distancesMeter, TYPE (*fieldAccessor)(const MetarParser& parser))
{
  // Calculate normalization factor by summarizing lambda for valid values
  double normFactor = 0.;
  for(int i = 0; i < metars.size(); i++)
  {
    if(valid(fieldAccessor(metars.at(i))))
      normFactor += lambda(atools::geo::meterToNm(distancesMeter.at(i)));
  }

  // Inverse factor - avoid division by zero
  normFactor = normFactor > 0.000001f ? 1. / normFactor : 10000000.;

  // Summarize all values ======================
  TYPE sum = 0;
  for(int i = 0; i < metars.size(); i++)
  {
    // Lambda for value
    double lamb = lambda(atools::geo::meterToNm(distancesMeter.at(i)));

    // Get value from metar list by callback
    TYPE type = fieldAccessor(metars.at(i));
    if(valid(type))
      sum += TYPE(type * lamb);
  }
  return TYPE(sum * normFactor);
}

atools::fs::weather::MetarParser MetarParser::merge(const atools::fs::weather::MetarParserList& metars,
                                                    const QList<float>& distancesMeter)
{
  MetarParser retval;
  retval.resetParsed();

  if(!metars.isEmpty())
  {
    // Copy all values over from nearest
    retval = metars.constFirst();
    strcpy(retval._icao, NO_IDENT.latin1());

    // Reset values where needed
    retval._temp = retval._dewp = retval._pressure = retval._gust_speed = INVALID_METAR_VALUE;
    retval._rain = retval._snow = -1;
    retval._wind_dir = retval._wind_range_from = -1;
    retval._wind_speed = retval._gust_speed = INVALID_METAR_VALUE;
    retval._min_visibility = retval._max_visibility = retval._vert_visibility = MetarVisibility();
    for(int i = 0; i < 8; i++)
      retval._dir_visibility[i] = MetarVisibility();

    // Collect ids and assigne latest timestamp to interpolated =================
    QStringList ids;
    for(int i = 0; i < metars.size(); i++)
    {
      const MetarParser& metar = metars.at(i);

      // Date latest
      if(!retval.getTimestamp().isValid() || retval.getTimestamp() < metar.getTimestamp())
      {
        retval._year = metar._year;
        retval._month = metar._month;
        retval._day = metar._day;
        retval._hour = metar._hour;
        retval._minute = metar._minute;
      }

      ids.append(metar.getId());
    }

    // Ignored values
    // MetarVisibility _max_visibility
    // int _wind_range_from;
    // int _wind_range_to;
    // MetarVisibility _vert_visibility;
    // MetarVisibility _dir_visibility[8];

    // Interpolate wind U/V components and convert them back to speed/degree =====================
    double windU = mergeField<double>(metars, distancesMeter, [](const MetarParser& m)->double {
            if(valid(m._wind_speed) && valid(m._wind_dir))
              return atools::geo::windUComponent(m._wind_speed, m._wind_dir);
            else
              return INVALID_METAR_VALUE;
          });

    double windV = mergeField<double>(metars, distancesMeter, [](const MetarParser& m)->double {
            if(valid(m._wind_speed) && valid(m._wind_dir))
              return atools::geo::windVComponent(m._wind_speed, m._wind_dir);
            else
              return INVALID_METAR_VALUE;
          });

    // float windVariable = mergeField<float>(metars, distancesMeter, [](const MetarParser& m)->float {
    // if(valid(m._wind_range_from) && valid(m._wind_range_to) && valid(m._wind_speed))
    // return m._wind_speed;
    // else
    // return INVALID_METAR_VALUE;
    // });

    retval._wind_speed = atools::geo::windSpeedFromUV(windU, windV);
    retval._wind_dir = atools::roundToInt(atools::geo::windDirectionFromUV(windU, windV));
    retval._wind_dir = atools::geo::normalizeRange(retval._wind_dir, 0, 360);

    // Interpolate gusts as scalar and not vector =============
    retval._gust_speed = mergeField<float>(metars, distancesMeter, [](const MetarParser& m)->float {return m._gust_speed;});

    // Temperature and dewpoint ==============================================================
    retval._temp = mergeField<float>(metars, distancesMeter, [](const MetarParser& m)->float {return m._temp;});

    retval._dewp = mergeField<float>(metars, distancesMeter, [](const MetarParser& m)->float {return m._dewp;});

    // Pressure =============================================================================
    retval._pressure = mergeField<float>(metars, distancesMeter, [](const MetarParser& m)->float {return m._pressure;});

    // Precipitation ============================================================================
    retval._rain = atools::roundToInt(mergeField<float>(metars, distancesMeter, [](const MetarParser& m)->float {
            return static_cast<float>(m._rain);
          }));

    retval._snow = atools::roundToInt(mergeField<float>(metars, distancesMeter, [](const MetarParser& m)->float {
            return static_cast<float>(m._snow);
          }));

    // Visibility - only min and equals used ==================================================================
    retval._min_visibility.visibilityMeter = atools::roundToInt(mergeField<float>(metars, distancesMeter, [](const MetarParser& m)->float {
            return m._min_visibility.visibilityMeter;
          }));
    retval._min_visibility.direction = -1;
    retval._min_visibility.modifier = MetarVisibility::EQUALS;
    retval._min_visibility.tendency = MetarVisibility::NONE;

    retval._max_visibility.visibilityMeter = INVALID_METAR_VALUE;
    retval._max_visibility.direction = -1;
    retval._max_visibility.modifier = MetarVisibility::EQUALS;
    retval._max_visibility.tendency = MetarVisibility::NONE;

    // Interpolate lowest cloud and maximum cover altitude ===================================
    retval.lowestCoverageCloud.altitudeMeter = mergeField<float>(metars, distancesMeter, [](const MetarParser& m)->float {
            return m.lowestCoverageCloud.altitudeMeter;
          });

    retval.maxCoverageCloud.altitudeMeter = mergeField<float>(metars, distancesMeter, [](const MetarParser& m)->float {
            return m.maxCoverageCloud.altitudeMeter;
          });

    // Interpolate lowest cloud and maximum coverage ===================================
    retval.lowestCoverageCloud.coverage =
      static_cast<MetarCloud::Coverage>(mergeField<int>(metars, distancesMeter, [](const MetarParser& m)->int {
            return static_cast<int>(m.lowestCoverageCloud.coverage);
          }));

    retval.maxCoverageCloud.coverage =
      static_cast<MetarCloud::Coverage>(mergeField<int>(metars, distancesMeter, [](const MetarParser& m)->int {
            return static_cast<int>(m.maxCoverageCloud.coverage);
          }));

    // Calculate flight rules from cloud coverage above =====================================
    retval.postProcessFlightRules();

    // Calculate prevailing wind from wind set above ===========================
    retval.postProcessPrevailingWind();

    // Adjust visibility distance steps ======================================
    retval._min_visibility.adjustDistance();

    if(retval._wind_speed < 1.f)
    {
      retval._wind_speed = 0.f;
      retval._wind_dir = -1;
    }

    // Do not interpolate clouds and add only coverage =========================
    retval._clouds.clear();
    if(retval.maxCoverageCloud.coverage != MetarCloud::COVERAGE_NIL)
      retval._clouds.push_back(retval.maxCoverageCloud);

    if(retval.lowestCoverageCloud.coverage != MetarCloud::COVERAGE_NIL)
      retval._clouds.push_back(retval.lowestCoverageCloud);

    // Sort by altitude
    std::sort(retval._clouds.begin(), retval._clouds.end(), [](const MetarCloud& cloud1, const MetarCloud& cloud2) {
            return cloud1.altitudeMeter < cloud2.altitudeMeter;
          });

    // Remove duplicates
    retval._clouds.erase(std::unique(retval._clouds.begin(), retval._clouds.end(), [](const MetarCloud& cloud1, const MetarCloud& cloud2) {
            return atools::almostEqual(cloud1.altitudeMeter, cloud2.altitudeMeter, 100.f);
          }), retval._clouds.end());

    // Build METAR string from content and assign it ===========================================================
    // Caller has to use parse() to get valid values again
    retval.setMetar(retval.buildMetarString(QStringList()));
  }

  // Indicate no cloud interpolation to caller
  retval.incompleteInterpolation = true;
  return retval;
}

QString MetarParser::buildMetarString(const QStringList& remarks) const
{
  QStringList metarStr;
  // KSFO 061656Z 19004KT 9SM SCT100 OVC200 08/03 A3013

  // Airport ====================================
  metarStr.append(getId());

  // Time ====================================
  metarStr.append(QString("%1%2%3Z").arg(_day, 2, 10, QChar('0')).arg(_hour, 2, 10, QChar('0')).arg(_minute, 2, 10, QChar('0')));

  // Wind ====================================
  if(_wind_dir != -1 && _wind_speed > 0.f && _wind_speed < INVALID_METAR_VALUE)
    metarStr.append(QString("%1%2KT").arg(_wind_dir, 3, 10, QChar('0')).
                    arg(atools::geo::meterPerSecToKnots(_wind_speed), 2, 'f', 0, QChar('0')));
  else
    metarStr.append("00000KT");

  // Visibility ====================================
  if(_min_visibility.isDistanceValid())
    metarStr.append(QString("%1").arg(_min_visibility.visibilityMeter, 4, 'f', 0, QChar('0')));

  // Precipitation ====================================
  metarStr.append(getIntensityStringShort(_rain, "RA"));
  metarStr.append(getIntensityStringShort(_snow, "SN"));

  // Clouds ==========================================
  for(const MetarCloud& cloud : _clouds)
  {
    QString coverageStr = cloud.getCoverageStringShort();

    if(!coverageStr.isEmpty())
      metarStr.append(coverageStr + QString("%1").arg(atools::geo::meterToFeet(cloud.altitudeMeter) / 100.f, 3, 'f', 0, QChar('0')));
  }

  // Temperature ====================================
  if(_temp < INVALID_METAR_VALUE && _dewp < INVALID_METAR_VALUE)
    metarStr.append(QString("%1%2/%3%4").
                    arg(_temp <= -0.5f ? "M" : QString()).arg(std::abs(_temp), 2, 'f', 0, QChar('0')).
                    arg(_dewp <= -0.5f ? "M" : QString()).arg(std::abs(_dewp), 2, 'f', 0, QChar('0')));

  // Pressure =============================================
  if(_pressure > 0.f && _pressure < INVALID_METAR_VALUE)
    metarStr.append(QString("Q%1").arg(getPressureMbar(), 4, 'f', 0, QChar('0')));

  metarStr.append(remarks);

  metarStr.removeAll(QString());
  return metarStr.join(' ');
}

QString MetarParser::getIntensityStringShort(int intensityValue, const QString& weatherPhenomenon) const
{
  switch(static_cast<Intensity>(intensityValue))
  {
    case NIL:
      break;

    case LIGHT:
      return "-" + weatherPhenomenon;

    case MODERATE:
      return weatherPhenomenon;

    case HEAVY:
      return "+" + weatherPhenomenon;
  }
  return QString();
}

/*
 * Takes a Metar string
 * The "METAR"
 * keyword has no effect (apart from incrementing the group counter
 * @a grpcount) and can be left away. A keyword "SPECI" is
 * likewise accepted.
 *
 * @param m     ICAO station id or metar string
 *
 * @par Examples:
 * @code
 * SGMetar *m = new SGMetar("METAR KSFO 061656Z 19004KT 9SM SCT100 OVC200 08/03 A3013");
 * double t = m->getTemperature_F();
 * delete m;
 *
 * @endcode
 */
void MetarParser::parse()
{
  resetParsed();

  if(metar.isEmpty())
    return;

  QByteArray utf8 = metar.toUtf8();
  _data = new char[utf8.size() + 2]; // make room for " \0"
  strcpy(_data, utf8.constData());

  normalizeData();

  _m = _data;
  _icao[0] = '\0';

  bool correctDate = false;
  // NOAA preample
  if(!scanPreambleDate())
  {
    useCurrentDate();
    correctDate = true;
  }
  scanPreambleTime();

  // METAR header
  scanType();
  if(!scanId() || !scanDate())
  {
    delete[] _data;
    _data = nullptr;
    errors.append(tr("METAR \"%1\" has errors.").arg(metar));
    return;
  }
  scanModifier();

  // base set
  scanWind();
  scanVariability();
  while(scanVisibility())
    ;

  while(scanRwyVisRange())
    ;

  while(scanWeather())
    ;

  while(scanSkyCondition())
    ;

  scanTemperature();
  scanPressure();
  while(scanSkyCondition())
    ;

  while(scanRunwayReport())
    ;

  scanWindShear();

  // appendix
  while(scanColorState())
    ;

  scanTrendForecast();
  while(scanRunwayReport())
    ;

  scanRemainder();
  scanRemark();

  if(groupCount < 4)
  {
    delete[] _data;
    _data = nullptr;
    errors.append(tr("METAR \"%1\" is incomplete.").arg(metar));
    return;
  }

  if(correctDate)
  {
    // Not full date parsed - find based on current time
    QDateTime dateTime = atools::correctDate(_day, _hour, _minute);
    _month = dateTime.date().month();
    _day = dateTime.date().day();
    _year = dateTime.date().year();
  }

  // Calculate flight rules (IFR, VFR, etc.), max and lowest ceiling
  postProcess();

  delete[] _data;
  _data = nullptr;
  unusedData = _m;
  _m = nullptr;

  parsed = true;
}

void MetarParser::postProcessCloudCoverage()
{
  const QList<MetarCloud> clouds = getClouds();

  // The lowest "BKN" or "OVC" layer specifies the cloud ceiling.
  float lowestAltitudeMeter = INVALID_METAR_VALUE;

  // Calculate lowest and maximum coverage
  maxCoverageCloud = lowestCoverageCloud = MetarCloud(MetarCloud::COVERAGE_CLEAR, 0.f);
  for(const MetarCloud& cloud : clouds)
  {
    MetarCloud::Coverage coverage = cloud.getCoverage();

    if(coverage > maxCoverageCloud.coverage)
      maxCoverageCloud = cloud;

    if(cloud.getAltitudeMeter() < lowestAltitudeMeter)
    {
      lowestAltitudeMeter = cloud.getAltitudeMeter();
      lowestCoverageCloud = cloud;
    }
  }
}

void MetarParser::postProcessFlightRules()
{
  // Calculate the flight rules depending on ceiling and visiblity
  float ceilingFt = atools::geo::meterToFeet(lowestCloudBase());
  float visibilityMi = atools::geo::meterToMi(getMinVisibility().getVisibilityMeter());

  if(visibilityMi < 1.f || ceilingFt < 500.f)
    flightRules = LIFR;
  else if(visibilityMi < 3.f || ceilingFt < 1000.f)
    flightRules = IFR;
  else if(visibilityMi <= 5.f || ceilingFt <= 3000.f)
    flightRules = MVFR;
  else
    flightRules = VFR;
}

void MetarParser::postProcessPrevailingWind()
{
  if(getWindDir() >= 0)
    prevailingWindDir = getWindDir();
  else if(getWindRangeFrom() != -1 && getWindRangeTo() != -1)
  {
    // Calculate the average wind direction based on variable wind
    int from = getWindRangeFrom(), to;
    if(getWindRangeFrom() < getWindRangeTo())
      to = getWindRangeTo();
    else
      to = getWindRangeTo() + 360;
    prevailingWindDir = atools::roundToInt(atools::geo::normalizeCourse(from + (to - from) / 2.));
  }

  prevailingWindSpeed = getWindSpeedMeterPerSec();
}

void MetarParser::postProcess()
{
  postProcessCloudCoverage();
  postProcessFlightRules();
  postProcessPrevailingWind();
}

float MetarParser::lowestCloudBase() const
{
  float minAltitudeMeter = INVALID_METAR_VALUE;

  // Calculate lowest and maximum coverage
  for(const MetarCloud& cloud : getClouds())
  {
    // The lowest "BKN" or "OVC" layer specifies the cloud ceiling.
    MetarCloud::Coverage coverage = cloud.getCoverage();
    if((coverage == MetarCloud::COVERAGE_BROKEN || coverage == MetarCloud::COVERAGE_OVERCAST) &&
       cloud.getAltitudeMeter() < minAltitudeMeter)
      minAltitudeMeter = cloud.getAltitudeMeter();
  }
  return minAltitudeMeter;
}

QString MetarParser::getReportTypeString() const
{
  ReportType t = static_cast<ReportType>(_report_type);
  switch(t)
  {
    case atools::fs::weather::MetarParser::NONE:
      return QString();

    case atools::fs::weather::MetarParser::AUTO:
      return tr("Auto");

    case atools::fs::weather::MetarParser::COR:
      return tr("Corrected");

    case atools::fs::weather::MetarParser::RTD:
      return tr("Routine delayed observation");

  }
  return QString();
}

float MetarParser::getWindSpeedKts() const
{
  if(_wind_speed < INVALID_METAR_VALUE / 2.f)
    return _wind_speed / SG_KT_TO_MPS;
  else
    return INVALID_METAR_VALUE;
}

float MetarParser::getGustSpeedKts() const
{
  if(_gust_speed < INVALID_METAR_VALUE / 2.f)
    return _gust_speed / SG_KT_TO_MPS;
  else
    return INVALID_METAR_VALUE;
}

QString MetarParser::getIntensityString(int intensity) const
{
  Intensity i = static_cast<Intensity>(intensity);
  switch(i)
  {
    case atools::fs::weather::MetarParser::NIL:
      return QString();

    case atools::fs::weather::MetarParser::LIGHT:
      return tr("Light");

    case atools::fs::weather::MetarParser::MODERATE:
      return tr("Moderate");

    case atools::fs::weather::MetarParser::HEAVY:
      return tr("Heavy");
  }
  return QString();
}

void MetarParser::useCurrentDate()
{
  struct tm now;

  time_t now_sec = time(nullptr);
#ifdef _WIN32
  now = *gmtime(&now_sec);
#else
  gmtime_r(&now_sec, &now);
#endif
  _year = now.tm_year + 1900;
  _month = now.tm_mon + 1;
}

/*
 * Replace any number of subsequent spaces by just one space, and add
 * a trailing space. This makes scanning for things like "ALL RWY" easier.
 */
void MetarParser::normalizeData()
{
  char *src, *dest;
  for(src = dest = _data; (*dest++ = *src++);)
    while(*src == ' ' && src[1] == ' ')
      src++;
  for(dest--; isspace(*--dest);)
    ;
  *++dest = ' ';
  *++dest = '\0';
}

// \d\d\d\d/\d\d/\d\d
bool MetarParser::scanPreambleDate()
{
  char *m = _m;
  int year, month, day;
  if(!scanNumber(&m, &year, 4))
    return false;

  if(*m++ != '/')
    return false;

  if(!scanNumber(&m, &month, 2))
    return false;

  if(*m++ != '/')
    return false;

  if(!scanNumber(&m, &day, 2))
    return false;

  if(!scanBoundary(&m))
    return false;

  _year = year;
  _month = month;
  _day = day;
  _m = m;
  return true;
}

// \d\d:\d\d
bool MetarParser::scanPreambleTime()
{
  char *m = _m;
  int hour, minute;
  if(!scanNumber(&m, &hour, 2))
    return false;

  if(*m++ != ':')
    return false;

  if(!scanNumber(&m, &minute, 2))
    return false;

  if(!scanBoundary(&m))
    return false;

  _hour = hour;
  _minute = minute;
  _m = m;
  return true;
}

// (METAR|SPECI)
bool MetarParser::scanType()
{
  if(strncmp(_m, "METAR ", 6) && strncmp(_m, "SPECI ", 6))
    return false;

  _m += 6;
  groupCount++;
  return true;
}

// [A-Z]{4}
bool MetarParser::scanId()
{
  char *m = _m;
  for(int i = 0; i < 4; m++, i++)
  {
    if(isspace(*m))
      break;
    if(!(isalpha(*m) || isdigit(*m)))
      return false;
  }

  if(!scanBoundary(&m))
    return false;

  strncpy(_icao, _m, 4);
  _icao[4] = '\0';
  _m = m;
  groupCount++;
  return true;
}

// \d{6}Z
bool MetarParser::scanDate()
{
  char *m = _m;
  int day, hour, minute;
  if(!scanNumber(&m, &day, 2))
    return false;

  if(!scanNumber(&m, &hour, 2))
    return false;

  if(!scanNumber(&m, &minute, 2))
    return false;

  if(*m++ != 'Z')
    return false;

  if(!scanBoundary(&m))
    return false;

  _day = day;
  _hour = hour;
  _minute = minute;
  _m = m;
  groupCount++;
  return true;
}

// (NIL|AUTO|COR|RTD)
bool MetarParser::scanModifier()
{
  char *m = _m;
  int type;
  if(!strncmp(m, "NIL", 3))
  {
    _m += strlen(_m);
    return true;
  }
  if(!strncmp(m, "AUTO", 4)) // automatically generated
    m += 4, type = AUTO;
  else if(!strncmp(m, "COR", 3)) // manually corrected
    m += 3, type = COR;
  else if(!strncmp(m, "CCA", 3)) // manually corrected for Canada
    m += 3, type = COR;
  else if(!strncmp(m, "RTD", 3)) // routine delayed
    m += 3, type = RTD;
  else
    return false;

  if(!scanBoundary(&m))
    return false;

  _report_type = type;
  _m = m;
  groupCount++;
  return true;
}

// (\d{3}|VRB)\d{1,3}(G\d{2,3})?(KT|KMH|MPS)
bool MetarParser::scanWind()
{
  char *m = _m;
  int dir;
  if(!strncmp(m, "VRB", 3))
    m += 3, dir = -1;
  else if(!strncmp(_m, "///", 3))
    m += 3, dir = -1;
  else if(!scanNumber(&m, &dir, 3))
    return false;

  int i;
  if(!strncmp(_m, "//", 2))
    m += 2, i = -1;
  else if(!scanNumber(&m, &i, 2, 3))
    return false;

  double speed = i == -1 ? INVALID_METAR_VALUE : i;

  double gust = INVALID_METAR_VALUE;
  if(*m == 'G')
  {
    m++;
    if(!scanNumber(&m, &i, 2, 3))
      return false;

    gust = i;
  }
  double factor;
  if(!strncmp(m, "KT", 2))
    m += 2, factor = SG_KT_TO_MPS;
  else if(!strncmp(m, "KMH", 3))
    m += 3, factor = SG_KMH_TO_MPS;
  else if(!strncmp(m, "KPH", 3)) // ??
    m += 3, factor = SG_KMH_TO_MPS;
  else if(!strncmp(m, "MPS", 3))
    m += 3, factor = 1.0;
  else
    return false;

  if(!scanBoundary(&m))
    return false;

  _m = m;
  _wind_dir = dir;
  if(speed < INVALID_METAR_VALUE)
    _wind_speed = static_cast<float>(speed * factor);
  if(gust < INVALID_METAR_VALUE)
    _gust_speed = static_cast<float>(gust * factor);
  groupCount++;
  return true;
}

// \d{3}V\d{3}
bool MetarParser::scanVariability()
{
  if(!strncmp(_m, "///V/// ", 8)) // spec compliant?
  {
    _m += 8;
    groupCount++;
    return true;
  }

  char *m = _m;
  int from, to;
  if(!scanNumber(&m, &from, 3))
    return false;

  if(*m++ != 'V')
    return false;

  if(!scanNumber(&m, &to, 3))
    return false;

  if(!scanBoundary(&m))
    return false;

  _m = m;
  _wind_range_from = from;
  _wind_range_to = to;
  groupCount++;
  return true;
}

bool MetarParser::scanVisibility()
{
  // TODO: if only directed vis are given, do still set min/max
  if(!strncmp(_m, "//// ", 5)) // spec compliant?
  {
    _m += 5;
    groupCount++;
    return true;
  }

  if(!strncmp(_m, "////SM ", 7)) // spec compliant?
  {
    _m += 7;
    groupCount++;
    return true;
  }

  char *m = _m;
  double distance;
  int i, dir = -1;
  int modifier = MetarVisibility::EQUALS;
  // \d{4}(N|NE|E|SE|S|SW|W|NW)?
  if(scanNumber(&m, &i, 4))
  {
    if(strncmp(m, "NDV", 3) == 0)
    {
      m += 3; // tolerate NDV (no directional validation)
    }
    else if(*m == 'E')
    {
      m++, dir = 90;
    }
    else if(*m == 'W')
    {
      m++, dir = 270;
    }
    else if(*m == 'N')
    {
      m++;
      if(*m == 'E')
        m++, dir = 45;
      else if(*m == 'W')
        m++, dir = 315;
      else
        dir = 0;
    }
    else if(*m == 'S')
    {
      m++;
      if(*m == 'E')
        m++, dir = 135;
      else if(*m == 'W')
        m++, dir = 225;
      else
        dir = 180;
    }
    if(i == 0)
      i = 50, modifier = MetarVisibility::LESS_THAN;
    else if(i == 9999)
      i++, modifier = MetarVisibility::GREATER_THAN;
    distance = i;
  }
  else
  {
    // M?(\d{1,2}|\d{1,2}/\d{1,2}|\d{1,2} \d{1,2}/\d{1,2})(SM|KM)
    if(*m == 'M')
      m++, modifier = MetarVisibility::LESS_THAN;

    if(!scanNumber(&m, &i, 1, 2))
      return false;

    distance = i;

    if(*m == '/')
    {
      m++;
      if(!scanNumber(&m, &i, 1, 2))
        return false;

      distance /= i;
    }
    else if(*m == ' ')
    {
      m++;
      int denom;
      if(!scanNumber(&m, &i, 1, 2))
        return false;

      if(*m++ != '/')
        return false;

      if(!scanNumber(&m, &denom, 1, 2))
        return false;

      distance += (double)i / denom;
    }

    if(!strncmp(m, "SM", 2))
      distance *= SG_SM_TO_METER, m += 2;
    else if(!strncmp(m, "KM", 2))
      distance *= 1000, m += 2;
    else
      return false;
  }
  if(!scanBoundary(&m))
    return false;

  MetarVisibility *v;
  if(dir != -1)
    v = &_dir_visibility[dir / 45];
  else if(!(_min_visibility.visibilityMeter < INVALID_METAR_VALUE))
    v = &_min_visibility;
  else
    v = &_max_visibility;

  v->visibilityMeter = static_cast<float>(distance);
  v->modifier = static_cast<MetarVisibility::Modifier>(modifier);
  v->direction = dir;
  _m = m;
  groupCount++;
  return true;
}

// R\d\d[LCR]?/([PM]?\d{4}V)?[PM]?\d{4}(FT)?[DNU]?
bool MetarParser::scanRwyVisRange()
{
  char *m = _m;
  int i;
  MetarRunway r;

  if(*m++ != 'R')
    return false;

  if(!scanNumber(&m, &i, 2))
    return false;

  if(*m == 'L' || *m == 'C' || *m == 'R')
    m++;

  char id[4];
  strncpy(id, _m + 1, i = m - _m - 1);
  id[i] = '\0';

  if(*m++ != '/')
    return false;

  int from, to;
  if(*m == 'P')
    m++, r.minVisibility.modifier = MetarVisibility::GREATER_THAN;
  else if(*m == 'M')
    m++, r.minVisibility.modifier = MetarVisibility::LESS_THAN;
  if(!scanNumber(&m, &from, 4))
    return false;

  if(*m == 'V')
  {
    m++;
    if(*m == 'P')
      m++, r.maxVisibility.modifier = MetarVisibility::GREATER_THAN;
    else if(*m == 'M')
      m++, r.maxVisibility.modifier = MetarVisibility::LESS_THAN;
    if(!scanNumber(&m, &to, 4))
      return false;
  }
  else
    to = from;

  if(!strncmp(m, "FT", 2))
  {
    from = int(from * SG_FEET_TO_METER);
    to = int(to * SG_FEET_TO_METER);
    m += 2;
  }
  r.minVisibility.visibilityMeter = from;
  r.maxVisibility.visibilityMeter = to;

  if(*m == '/') // this is not in the spec!
    m++;
  if(*m == 'D')
    m++, r.minVisibility.tendency = MetarVisibility::DECREASING;
  else if(*m == 'N')
    m++, r.minVisibility.tendency = MetarVisibility::STABLE;
  else if(*m == 'U')
    m++, r.minVisibility.tendency = MetarVisibility::INCREASING;

  if(!scanBoundary(&m))
    return false;

  _m = m;

  _runways[id].minVisibility = r.minVisibility;
  _runways[id].maxVisibility = r.maxVisibility;
  groupCount++;
  return true;
}

// (+|-|VC)?(NSW|MI|PR|BC|DR|BL|SH|TS|FZ)?((DZ|RA|SN|SG|IC|PE|GR|GS|UP){0,3})(BR|FG|FU|VA|DU|SA|HZ|PY|PO|SQ|FC|SS|DS){0,3}
bool MetarParser::scanWeather()
{
  char *m = _m;
  string weather;
  const struct Token *a;

  if(!strncmp(m, "NOSIG ", 6))
  {
    _m += 6;
    groupCount++;
    return false;
  }

  // @see WMO-49 Section 4.4.2.9
  // Denotes a temporary failure of the sensor
  if(!strncmp(m, "// ", 3))
  {
    _m += 3;
    groupCount++;
    return false;
  }

  if((a = scanToken(&m, special)))
  {
    if(!scanBoundary(&m))
      return false;

    _weather.push_back(a->text.toStdString());
    _m = m;
    return true;
  }

  string pre, post;
  struct Weather w;

  if(*m == '-')
    m++, pre = tr("Light ").toStdString(), w.intensity = LIGHT;
  else if(*m == '+')
    m++, pre = tr("Heavy ").toStdString(), w.intensity = HEAVY;
  else if(!strncmp(m, "VC", 2))
    m += 2, post = tr("in the vicinity ").toStdString(), w.vincinity = true;
  else
    pre = tr("Moderate ").toStdString(), w.intensity = MODERATE;

  int i;
  for(i = 0; i < 3; i++)
  {
    if(!(a = scanToken(&m, description)))
      break;
    w.descriptions.push_back(a->id);
    weather += a->text.toStdString() + " ";
  }

  for(i = 0; i < 3; i++)
  {
    if(!(a = scanToken(&m, phenomenon)))
      break;
    w.phenomena.push_back(a->id);
    weather += a->text.toStdString() + " ";
    if(a->id == "RA")
      _rain = w.intensity;
    else if(a->id == "HA")
      _hail = w.intensity;
    else if(a->id == "SN")
      _snow = w.intensity;
  }
  if(!weather.length())
    return false;

  if(!scanBoundary(&m))
    return false;

  _m = m;

  weather = pre + weather + post;
  weather.erase(weather.length() - 1);
  _weather.push_back(weather);
  if(!w.phenomena.empty())
  {
    _weather2.push_back(w);
  }
  groupCount++;
  return true;
}

// (FEW|SCT|BKN|OVC|SKC|CLR|CAVOK|VV)([0-9]{3}|///)?[:cloud_type:]?
bool MetarParser::scanSkyCondition()
{
  char *m = _m;
  int i;
  MetarCloud cl;

  if(!strncmp(m, "//////TCU", 9))
  {
    m += 9;
    if(!scanBoundary(&m))
      return false;

    _m = m;
    return true;
  }

  if(!strncmp(m, "//////CB", 8))
  {
    m += 8;
    if(!scanBoundary(&m))
      return false;

    _m = m;
    return true;
  }

  if(!strncmp(m, "///CB", 5))
  {
    m += 5;
    if(!scanBoundary(&m))
      return false;

    _m = m;
    return true;
  }

  if(!strncmp(m, "/////////", 9))
  {
    m += 9;
    if(!scanBoundary(&m))
      return false;

    _m = m;
    return true;
  }

  if(!strncmp(m, "//////", 6))
  {
    m += 6;
    if(!scanBoundary(&m))
      return false;

    _m = m;
    return true;
  }

  if(!strncmp(m, "/////", 5))
  {
    m += 5;
    if(!scanBoundary(&m))
      return false;

    _m = m;
    return true;
  }

  if(!strncmp(m, "CLR", i = 3) || // clear
     !strncmp(m, "SKC", i = 3) || // sky clear
     !strncmp(m, "NCD", i = 3) || // nil cloud detected
     !strncmp(m, "NSC", i = 3) || // no significant clouds
     !strncmp(m, "NOSIG", i = 5) || // no significant clouds
     !strncmp(m, "CAVOK 9999", i = 10) || // ceiling and visibility OK 9999 sometimes given too
     !strncmp(m, "CAVOK", i = 5))    // ceiling and visibility OK (implies 9999)
  {
    m += i;
    if(!scanBoundary(&m))
      return false;

    if(i == 3)
    {
      cl.coverage = MetarCloud::COVERAGE_CLEAR;
      _clouds.push_back(cl);
    }
    else
    {
      _cavok = true;
    }
    _m = m;
    return true;
  }

  bool unknown = false;
  if(!strncmp(m, "VV", i = 2)) // vertical visibility
    ;
  else if(!strncmp(m, "FEW", i = 3))
    cl.coverage = MetarCloud::COVERAGE_FEW;
  else if(!strncmp(m, "SCT", i = 3))
    cl.coverage = MetarCloud::COVERAGE_SCATTERED;
  else if(!strncmp(m, "BKN", i = 3))
    cl.coverage = MetarCloud::COVERAGE_BROKEN;
  else if(!strncmp(m, "OVC", i = 3))
    cl.coverage = MetarCloud::COVERAGE_OVERCAST;
  else if(!strncmp(m, "///", i = 3))
  {
    unknown = true;
    cl.coverage = MetarCloud::COVERAGE_NIL;
  }
  else
    return false;

  m += i;

  if(!strncmp(m, "///", 3)) // vis not measurable (e.g. because of heavy snowing)
    m += 3, i = -1;
  else if(scanBoundary(&m))
  {
    _m = m;
    return true; // ignore single OVC/BKN/...
  }
  else if(!scanNumber(&m, &i, 2, 3))
    i = -1;

  if(cl.coverage == MetarCloud::COVERAGE_NIL && !unknown)
  {
    if(!scanBoundary(&m))
      return false;

    if(i == -1) // 'VV///'
      _vert_visibility.modifier = MetarVisibility::NOGO;
    else
      _vert_visibility.visibilityMeter = i * 100 * SG_FEET_TO_METER;
    _m = m;
    return true;
  }

  if(i != -1)
    cl.altitudeMeter = i * 100 * SG_FEET_TO_METER;

  const struct Token *a;

  if((a = scanToken(&m, cloud_types)))
  {
    cl.type = a->id;
    cl.typeLong = a->text;
  }

  // @see WMO-49 Section 4.5.4.5
  // Denotes temporary failure of sensor and covers cases like FEW045///
  if(!strncmp(m, "///", 3))
    m += 3;
  if(!scanBoundary(&m))
    return false;

  _clouds.push_back(cl);

  _m = m;
  groupCount++;
  return true;
}

// M?[0-9]{2}/(M?[0-9]{2})?            (spec)
// (M?[0-9]{2}|XX)/(M?[0-9]{2}|XX)?    (Namibia)
bool MetarParser::scanTemperature()
{
  char *m = _m;
  int sign = 1, temp, dew;
  if(!strncmp(m, "XX/XX", 5)) // not spec compliant!
  {
    _m += 5;
    return scanBoundary(&_m);
  }

  if(!strncmp(m, "/////", 5))
  {
    _m += 5;
    return scanBoundary(&_m);
  }

  if(*m == 'M')
    m++, sign = -1;
  if(!scanNumber(&m, &temp, 2))
    return false;

  temp *= sign;

  if(*m == '/')
    m++;

  if(!scanBoundary(&m))
  {
    if(!strncmp(m, "//", 2)) // Iceland no humidity
      m += 2, sign = 0, dew = temp;
    else if(!strncmp(m, "XX", 2)) // not spec compliant!
      m += 2, sign = 0, dew = temp;
    else
    {
      sign = 1;
      if(*m == 'M')
        m++, sign = -1;
      if(!scanNumber(&m, &dew, 2))
        return false;
    }
    if(!scanBoundary(&m))
      return false;

    if(sign)
      _dewp = sign * dew;
  }
  _temp = temp;
  _m = m;
  groupCount++;
  return true;
}

double MetarParser::getRelHumidity() const
{
  if(!(_temp < INVALID_METAR_VALUE) || !(_dewp < INVALID_METAR_VALUE))
    return INVALID_METAR_VALUE;

  double dewp = pow(10.0, 7.5 * _dewp / (237.7 + _dewp));
  double temp = pow(10.0, 7.5 * _temp / (237.7 + _temp));
  return dewp * 100 / temp;
}

const QHash<QString, MetarRunway> MetarParser::getRunways() const
{
  QHash<QString, MetarRunway> retval;

  for(const auto& keyval : _runways)
    retval.insert(QString::fromStdString(keyval.first), keyval.second);

  return retval;
}

const QStringList MetarParser::getWeather() const
{
  QStringList retval;

  for(const std::string& str : _weather)
    retval.append(QString::fromStdString(str));
  return retval;
}

QString MetarParser::getFlightRulesStringLong() const
{
  switch(flightRules)
  {
    case atools::fs::weather::MetarParser::UNKNOWN:
      return QString();

    case atools::fs::weather::MetarParser::LIFR:
      return tr("Low Instrument Flight Rules");

    case atools::fs::weather::MetarParser::IFR:
      return tr("Instrument Flight Rules");

    case atools::fs::weather::MetarParser::MVFR:
      return tr("Marginal Visual Flight Rules");

    case atools::fs::weather::MetarParser::VFR:
      return tr("Visual Flight Rules");
  }
  return QString();
}

QString MetarParser::getFlightRulesString() const
{
  switch(flightRules)
  {
    case atools::fs::weather::MetarParser::UNKNOWN:
      return QString();

    case atools::fs::weather::MetarParser::LIFR:
      return tr("LIFR");

    case atools::fs::weather::MetarParser::IFR:
      return tr("IFR");

    case atools::fs::weather::MetarParser::MVFR:
      return tr("MVFR");

    case atools::fs::weather::MetarParser::VFR:
      return tr("VFR");
  }
  return QString();
}

float MetarParser::getPrevailingWindSpeedKnots() const
{
  if(prevailingWindSpeed < INVALID_METAR_VALUE / 2.f)
    return prevailingWindSpeed / SG_KT_TO_MPS;
  else
    return INVALID_METAR_VALUE;
}

// [AQ]\d{4}             (spec)
// [AQ]\d{2}(\d{2}|//)   (Namibia)
bool MetarParser::scanPressure()
{
  char *m = _m;
  double factor;
  int press, i;

  bool slp = false;
  if(*m == 'A')
    factor = SG_INHG_TO_PA / 100;
  else if(*m == 'Q')
    factor = 100;
  // else if(strncmp(m, "SLP", 3))
  // {
  // factor = SG_INHG_TO_PA / 100;
  // slp = true;
  // }
  else
    return false;

  m += slp ? 3 : 1;

  if(!scanNumber(&m, &press, slp ? 3 : 2))
    return false;

  press *= 100;
  if(!strncmp(m, "//", 2)) // not spec compliant!
    m += 2;
  else if(scanNumber(&m, &i, 2))
    press += i;
  else
    return false;

  _pressure = press * factor;

  if(!scanBoundary(&m))
    return false;

  _m = m;
  groupCount++;
  return true;
}

// \d\d(CLRD|[\d/]{4})(\d\d|//)
bool MetarParser::scanRunwayReport()
{
  char *m = _m;
  int i;
  char id[4];
  MetarRunway r;

  if(!scanNumber(&m, &i, 2))
    return false;

  if(i == 88)
    strcpy(id, "ALL");
  else if(i == 99)
    strcpy(id, "REP"); // repetition of previous report
  else if(i >= 50)
  {
    i -= 50;
    id[0] = i / 10 + '0', id[1] = i % 10 + '0', id[2] = 'R', id[3] = '\0';
  }
  else
    id[0] = i / 10 + '0', id[1] = i % 10 + '0', id[2] = '\0';

  if(!strncmp(m, "CLRD", 4))
  {
    m += 4; // runway cleared
    r.depositString = MetarParser::tr("cleared");
  }
  else
  {
    if(scanNumber(&m, &i, 1))
    {
      r.deposit = i;
      r.depositString = runway_deposit[i];
    }
    else if(*m == '/')
      m++;
    else
      return false;

    if(*m == '1' || *m == '2' || *m == '5' || *m == '9') // extent of deposit
    {
      r.extent = *m - '0';
      r.extentString = runway_deposit_extent[*m - '0'];
    }
    else if(*m != '/')
      return false;

    m++;
    i = -1;
    if(!strncmp(m, "//", 2))
      m += 2;
    else if(!scanNumber(&m, &i, 2))
      return false;

    if(i == 0)
      r.depth = 0.0005; // < 1 mm deep (let's say 0.5 :-)
    else if(i > 0 && i <= 90)
      r.depth = i / 1000.0; // i mm deep
    else if(i >= 92 && i <= 98)
      r.depth = (i - 90) / 20.0;
    else if(i == 99)
      r.comment = MetarParser::tr("runway not in use");
    else if(i == -1) // no depth given ("//")
      ;
    else
      return false;
  }
  i = -1;
  if(m[0] == '/' && m[1] == '/')
    m += 2;
  else if(!scanNumber(&m, &i, 2))
    return false;

  if(i >= 1 && i < 90)
  {
    r.friction = i / 100.0;
  }
  else if((i >= 91 && i <= 95) || i == 99)
  {
    r.frictionString = runway_friction[i - 90];
  }
  if(!scanBoundary(&m))
    return false;

  _runways[id].deposit = r.deposit;
  _runways[id].depositString = r.depositString;
  _runways[id].extent = r.extent;
  _runways[id].extentString = r.extentString;
  _runways[id].depth = r.depth;
  _runways[id].friction = r.friction;
  _runways[id].frictionString = r.frictionString;
  _runways[id].comment = r.comment;
  _m = m;
  groupCount++;
  return true;
}

// WS (ALL RWYS?|RWY ?\d\d[LCR]?)?
bool MetarParser::scanWindShear()
{
  char *m = _m;
  if(strncmp(m, "WS", 2))
    return false;

  m += 2;
  if(!scanBoundary(&m))
    return false;

  if(!strncmp(m, "ALL", 3))
  {
    m += 3;
    if(!scanBoundary(&m))
      return false;

    if(strncmp(m, "RWY", 3))
      return false;

    m += 3;
    if(*m == 'S')
      m++;
    if(!scanBoundary(&m))
      return false;

    _runways["ALL"].windShear = true;
    _m = m;
    return true;
  }

  char id[4], *mm;
  int i, cnt;
  for(cnt = 0;; cnt++) // ??
  {
    if(strncmp(m, "RWY", 3))
      break;
    m += 3;
    scanBoundary(&m);
    mm = m;
    if(!scanNumber(&m, &i, 2))
      return false;

    if(*m == 'L' || *m == 'C' || *m == 'R')
      m++;
    strncpy(id, mm, i = m - mm);
    id[i] = '\0';
    if(!scanBoundary(&m))
      return false;

    _runways[id].windShear = true;
  }
  if(!cnt)
    _runways["ALL"].windShear = true;
  _m = m;
  return true;
}

bool MetarParser::scanTrendForecast()
{
  char *m = _m;
  if(strncmp(m, "NOSIG", 5))
    return false;

  m += 5;
  if(!scanBoundary(&m))
    return false;

  _m = m;
  return true;
}

bool MetarParser::scanColorState()
{
  char *m = _m;
  const struct Token *a;

  if(!(a = scanToken(&m, colors)))
    return false;

  if(!scanBoundary(&m))
    return false;

  // printf(Y"Code %s\n"N, a->text);
  _m = m;
  return true;
}

bool MetarParser::scanRemark()
{
  if(strncmp(_m, "RMK", 3))
    return false;

  _m += 3;
  if(!scanBoundary(&_m))
    return false;

  _remark = _m;

  while(*_m)
  {
    if(!scanRunwayReport())
    {
      while(*_m && !isspace(*_m))
        _m++;
      scanBoundary(&_m);
    }
  }
  return true;
}

bool MetarParser::scanRemainder()
{
  char *m = _m;
  if(!(strncmp(m, "NOSIG", 5)))
  {
    m += 5;
    if(scanBoundary(&m))
      _m = m; // _comment.push_back("No significant tendency");
  }

  if(!scanBoundary(&m))
    return false;

  _m = m;
  return true;
}

bool MetarParser::scanBoundary(char **s)
{
  if(**s && !isspace(**s))
    return false;

  while(isspace(**s))
    (*s)++;
  return true;
}

int MetarParser::scanNumber(char **src, int *num, int min, int max)
{
  int i;
  char *s = *src;
  *num = 0;
  for(i = 0; i < min; i++)
  {
    if(!isdigit(*s))
      return 0;
    else
      *num = *num * 10 + *s++ - '0';
  }
  for(; i < max && isdigit(*s); i++)
    *num = *num * 10 + *s++ - '0';
  *src = s;
  return i;
}

// find longest match of str in list
const struct Token *MetarParser::scanToken(char **str, const QList<Token>& list)
{
  const struct Token *longest = nullptr;
  int maxlen = 0;
  QString string(*str);

  for(const Token& token : list)
  {
    int len = token.id.size();
    if(string.left(len) == token.id && len > maxlen)
    {
      maxlen = len;
      longest = &token;
    }
  }

  // const char *s;
  // for(int i = 0; (s = list[i].id); i++)
  // {
  // len = strlen(s);
  // if(!strncmp(s, *str, len) && len > maxlen)
  // {
  // maxlen = len;
  // longest = &list[i];
  // }
  // }
  *str += maxlen;
  return longest;
}

void MetarCloud::set(float alt, Coverage cov)
{
  altitudeMeter = alt;
  if(cov != -1)
    coverage = cov;
}

QString MetarCloud::getCoverageString(MetarCloud::Coverage cloudCoverage)
{
  switch(cloudCoverage)
  {
    case atools::fs::weather::MetarCloud::COVERAGE_NIL:
      return QString();

    case atools::fs::weather::MetarCloud::COVERAGE_CLEAR:
      return tr("Clear");

    case atools::fs::weather::MetarCloud::COVERAGE_FEW:
      return tr("Few");

    case atools::fs::weather::MetarCloud::COVERAGE_SCATTERED:
      return tr("Scattered");

    case atools::fs::weather::MetarCloud::COVERAGE_BROKEN:
      return tr("Broken");

    case atools::fs::weather::MetarCloud::COVERAGE_OVERCAST:
      return tr("Overcast");
  }
  return QString();

}

QString MetarCloud::getCoverageStringShort(MetarCloud::Coverage cloudCoverage)
{
  switch(cloudCoverage)
  {
    case atools::fs::weather::MetarCloud::COVERAGE_NIL:
    case atools::fs::weather::MetarCloud::COVERAGE_CLEAR:
      return QString();

    case atools::fs::weather::MetarCloud::COVERAGE_FEW:
      return "FEW";

    case atools::fs::weather::MetarCloud::COVERAGE_SCATTERED:
      return "SCT";

    case atools::fs::weather::MetarCloud::COVERAGE_BROKEN:
      return "BKN";

    case atools::fs::weather::MetarCloud::COVERAGE_OVERCAST:
      return "OVC";
  }
  return QString();

}

MetarCloud::Coverage MetarCloud::getCoverage(const QString& coverage)
{
  if(coverage == QLatin1String("clear"))
    return COVERAGE_CLEAR;

  if(coverage == QLatin1String("few"))
    return COVERAGE_FEW;

  if(coverage == QLatin1String("scattered"))
    return COVERAGE_SCATTERED;

  if(coverage == QLatin1String("broken"))
    return COVERAGE_BROKEN;

  if(coverage == QLatin1String("overcast"))
    return COVERAGE_OVERCAST;

  return COVERAGE_NIL;
}

QString MetarVisibility::getModifierString() const
{
  Modifier mod = static_cast<Modifier>(modifier);
  switch(mod)
  {
    case atools::fs::weather::MetarVisibility::NOGO:
      return tr("Vertical visibility");

    case atools::fs::weather::MetarVisibility::EQUALS:
      return QString();

    case atools::fs::weather::MetarVisibility::LESS_THAN:
      return tr("Less than");

    case atools::fs::weather::MetarVisibility::GREATER_THAN:
      return tr("Greater than");
  }
  return QString();
}

void MetarVisibility::adjustDistance()
{
  if(visibilityMeter < INVALID_METAR_VALUE && visibilityMeter > 0.f)
  {
    if(visibilityMeter < 800.f)
      visibilityMeter = atools::floorToNearest(visibilityMeter, 50.f);
    else if(visibilityMeter < 5000.f)
      visibilityMeter = atools::floorToNearest(visibilityMeter, 100.f);
    else if(visibilityMeter < 10000.f)
      visibilityMeter = atools::floorToNearest(visibilityMeter, 1000.f);
    else
      visibilityMeter = 9999.f;
  }
}

QDateTime atools::fs::weather::MetarParser::getTimestamp() const
{
  return QDateTime(QDate(getYear(), getMonth(), getDay()), QTime(getHour(), getMinute()), QTimeZone::utc());
}

} // namespace weather
} // namespace fs
} // namespace atools
