// metar interface class
//
// Written by Melchior FRANZ, started December 2003.
//
// Copyright (C) 2003 Melchior FRANZ - mfranz@aon.at
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// $Id$

#ifndef ATOOLS_METAR_HXX
#define ATOOLS_METAR_HXX

#include <QCoreApplication>
#include <vector>
#include <map>
#include <string>
#include <QVector>

namespace atools {
namespace fs {
namespace weather {

/* Initialize static translateable texts */
void initTranslateableTexts();

struct Token
{
  QString id;
  QString text;
};

const Q_DECL_CONSTEXPR float INVALID_METAR_VALUE = std::numeric_limits<float>::max();

class MetarParser;

// ============================================================================
// ============================================================================
class MetarVisibility
{
  friend class MetarParser;
  Q_DECLARE_TR_FUNCTIONS(MetarVisibility)

public:
  MetarVisibility() :
    visibilityMeter(INVALID_METAR_VALUE), direction(-1), modifier(EQUALS), tendency(NONE)
  {
  }

  enum Modifier
  {
    NOGO,
    EQUALS,
    LESS_THAN,
    GREATER_THAN
  };

  enum Tendency
  {
    NONE,
    STABLE,
    INCREASING,
    DECREASING
  };

  float getVisibilityMeter() const
  {
    return visibilityMeter;
  }

  int getDirection() const
  {
    return direction;
  }

  Modifier getModifier() const
  {
    return modifier;
  }

  QString getModifierString() const;

  Tendency getTendency() const
  {
    return tendency;
  }

  bool isDistanceValid() const
  {
    return visibilityMeter < INVALID_METAR_VALUE;
  }

  /* Adjust distance according to METAR specs */
  void adjustDistance();

private:
  float visibilityMeter;
  int direction;
  Modifier modifier;
  Tendency tendency;
};

// ============================================================================
// ============================================================================
// runway condition (surface and visibility)
class MetarRunway
{
  friend class MetarParser;
  Q_DECLARE_TR_FUNCTIONS(MetarRunway)

public:
  MetarRunway() :
    deposit(-1), extent(-1), depth(INVALID_METAR_VALUE),
    friction(INVALID_METAR_VALUE), windShear(false)
  {
  }

  int getDeposit() const
  {
    return deposit;
  }

  const QString& getDepositString() const
  {
    return depositString;
  }

  float getExtent() const
  {
    return extent;
  }

  const QString& getExtentString() const
  {
    return extentString;
  }

  float getDepth() const
  {
    return depth;
  }

  float getFriction() const
  {
    return friction;
  }

  const QString& getFrictionString() const
  {
    return frictionString;
  }

  const QString& getComment() const
  {
    return comment;
  }

  bool getWindShear() const
  {
    return windShear;
  }

  const MetarVisibility& getMinVisibility() const
  {
    return minVisibility;
  }

  const MetarVisibility& getMaxVisibility() const
  {
    return maxVisibility;
  }

private:
  MetarVisibility minVisibility;
  MetarVisibility maxVisibility;
  int deposit;
  QString depositString;
  int extent;
  QString extentString;
  float depth;
  float friction;
  QString frictionString;
  QString comment;
  bool windShear;
};

// ============================================================================
// ============================================================================
// cloud layer
class MetarCloud
{
  friend class MetarParser;
  Q_DECLARE_TR_FUNCTIONS(MetarCloud)

public:
  enum Coverage
  {
    COVERAGE_NIL = -1,
    COVERAGE_CLEAR = 0,
    COVERAGE_FEW = 1,
    COVERAGE_SCATTERED = 2,
    COVERAGE_BROKEN = 3,
    COVERAGE_OVERCAST = 4
  };

  MetarCloud()
    : coverage(COVERAGE_NIL), altitudeMeter(INVALID_METAR_VALUE)
  {
  }

  MetarCloud(atools::fs::weather::MetarCloud::Coverage coverageParam, float altitudeParam)
    : coverage(coverageParam), altitudeMeter(altitudeParam)
  {
  }

  void set(float alt, Coverage cov = COVERAGE_NIL);

  Coverage getCoverage() const
  {
    return coverage;
  }

  static QString getCoverageString(Coverage cloudCoverage);

  QString getCoverageString() const
  {
    return getCoverageString(coverage);
  }

  static QString getCoverageStringShort(Coverage cloudCoverage);

  QString getCoverageStringShort() const
  {
    return getCoverageStringShort(coverage);
  }

  static Coverage getCoverage(const QString& coverage);

  float getAltitudeMeter() const
  {
    return altitudeMeter;
  }

  const QString& getTypeString() const
  {
    return type;
  }

  const QString& getTypeLongString() const
  {
    return typeLong;
  }

private:
  Coverage coverage; // quarters: 0 -> clear ... 4 -> overcast
  float altitudeMeter; // 1000 m
  QString type; // CU
  QString typeLong; // cumulus
};

// ============================================================================
// ============================================================================
class MetarParser;
typedef QVector<MetarParser> MetarParserVector;

class MetarParser
{
  Q_DECLARE_TR_FUNCTIONS(MetarParser)

public:
  MetarParser()
  {
  }

  /* Sets only metar but does not parse */
  explicit MetarParser(const QString& metarParam)
    : metar(metarParam)
  {
  }

  /* Sets only metar but does not parse */
  void setMetar(const QString& value)
  {
    if(metar != value)
    {
      resetParsed();
      metar = value;
    }
  }

  /* true if METAR was read successfully */
  bool isParsed() const
  {
    return parsed;
  }

  bool hasMetarString() const
  {
    return !metar.isEmpty();
  }

  /* Read METAR string and fill all values in object. isParsed() returns true if successfully parsed. */
  void parse();

  /* Reset all but METAR string */
  void resetParsed();

  /* FSX/P3D format needs preparating before parsing can be done */
  bool isFsxP3dFormat() const
  {
    return fsxP3dFormat;
  }

  void setFsxP3dFormat(bool value = true)
  {
    fsxP3dFormat = value;
  }

  /* METAR is a result of interpolation and contains no cloud information and others */
  bool isIncompleteInterpolation() const
  {
    return incompleteInterpolation;
  }

  const QString& getMetarString() const
  {
    return metar;
  }

  /* Removes XXXX from interpolated */
  const QString getMetarDisplayString() const;

  /* List is empty in case of success parsing */
  const QStringList& getErrors() const
  {
    return errors;
  }

  bool hasErrors() const
  {
    return !errors.isEmpty();
  }

  /* Interpolates weather between list "metars" and returns result.
   * Interpolates, wind, flight rules, visibility and pressure but not clouds.
   * metars has to be ordered by distance to origin. */
  static atools::fs::weather::MetarParser merge(const atools::fs::weather::MetarParserVector& metars,
                                                const QVector<float>& distancesMeter);

  enum FlightRules
  {
    UNKNOWN = -1,

    /* Magenta
     * Low Instrument Flight Rules (LIFR): Ceilings are less than 500 feet and/or visibility is less than 1 mile.
     * LIFR = <500′ and/or <1 mile*/
    LIFR = 0,

    /* Red
     * Instrument Flight Rules (IFR): Ceilings 500 to less than 1,000 feet and/or visibility 1 to less than 3 miles.
     * IFR = 500-1000′ and/or 1-3 miles */
    IFR = 1,

    /* Blue
     * Marginal VFR (MVFR): Ceilings 1,000 to 3,000 feet and/or visibility is 3-5 miles inclusive.
     * MVFR = 1000-3000′ and/or 3-5 miles */
    MVFR = 2,

    /* Green
     * VFR: Ceiling greater than 3000 feet and visibility greater than 5 miles(includes sky clear).
     * VFR = > 3000′ and > 5 miles*/
    VFR = 3
  };

  enum ReportType
  {
    NONE,
    AUTO,
    COR,
    RTD
  };

  enum Intensity
  {
    NIL = 0,
    LIGHT = 1,
    MODERATE = 2,
    HEAVY = 3
  };

  struct Weather
  {
    Weather()
    {
      intensity = NIL;
      vincinity = false;
    }

    Intensity intensity;
    bool vincinity;
    QStringList descriptions;
    QStringList phenomena;
  };

  QString getUnusedData() const
  {
    return QString::fromStdString(unusedData);
  }

  QString getId() const
  {
    return _icao;
  }

  int getYear() const
  {
    return _year;
  }

  int getMonth() const
  {
    return _month;
  }

  int getDay() const
  {
    return _day;
  }

  int getHour() const
  {
    return _hour;
  }

  int getMinute() const
  {
    return _minute;
  }

  QDateTime getTimestamp() const;

  int getReportType() const
  {
    return _report_type;
  }

  QString getReportTypeString() const;

  int getWindDir() const
  {
    return _wind_dir;
  }

  float getWindSpeedMeterPerSec() const
  {
    return _wind_speed;
  }

  float getWindSpeedKts() const;

  float getGustSpeedMeterPerSec() const
  {
    return _gust_speed;
  }

  float getGustSpeedKts() const;

  int getWindRangeFrom() const
  {
    return _wind_range_from;
  }

  int getWindRangeTo() const
  {
    return _wind_range_to;
  }

  const MetarVisibility& getMinVisibility() const
  {
    return _min_visibility;
  }

  const MetarVisibility& getMaxVisibility() const
  {
    return _max_visibility;
  }

  const MetarVisibility& getVertVisibility() const
  {
    return _vert_visibility;
  }

  const MetarVisibility *getDirVisibility() const
  {
    return _dir_visibility;
  }

  float getTemperatureC() const
  {
    return _temp;
  }

  float getDewpointDegC() const
  {
    return _dewp;
  }

  float getPressureMbar() const
  {
    return _pressure < INVALID_METAR_VALUE ? _pressure / 100 : INVALID_METAR_VALUE;
  }

  int getRain() const
  {
    return _rain;
  }

  int getHail() const
  {
    return _hail;
  }

  int getSnow() const
  {
    return _snow;
  }

  QString getIntensityString(int intensity) const;

  bool getCavok() const
  {
    return _cavok;
  }

  double getRelHumidity() const;

  const QList<MetarCloud> getClouds() const
  {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return QList<MetarCloud>(_clouds.begin(), _clouds.end());
#else
    return QVector<MetarCloud>::fromStdVector(_clouds).toList();
#endif
  }

  const QHash<QString, MetarRunway> getRunways() const;

  const QStringList getWeather() const;

  const QList<Weather> getWeather2() const
  {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return QList<Weather>(_weather2.begin(), _weather2.end());
#else
    return QVector<Weather>::fromStdVector(_weather2).toList();
#endif
  }

  QString getRemark() const
  {
    return QString::fromStdString(_remark);
  }

  FlightRules getFlightRules() const
  {
    return flightRules;
  }

  QString getFlightRulesStringLong() const;
  QString getFlightRulesString() const;

  /* Direction might be average of varable wind */
  int getPrevailingWindDir() const
  {
    return prevailingWindDir;
  }

  float getPrevailingWindSpeedMeterPerSec() const
  {
    return prevailingWindSpeed;
  }

  float getPrevailingWindSpeedKnots() const;

  /* Thickest cloud coverage */
  MetarCloud::Coverage getMaxCoverage() const
  {
    return maxCoverageCloud.coverage;
  }

  QString getMaxCoverageString() const
  {
    return MetarCloud::getCoverageString(maxCoverageCloud.coverage);
  }

  /* Coverage of lowest cloud layer */
  MetarCloud::Coverage getLowestCoverage() const
  {
    return lowestCoverageCloud.coverage;
  }

  QString getLowestCoverageString() const
  {
    return MetarCloud::getCoverageString(lowestCoverageCloud.coverage);
  }

  /* Empty instance which can be returned in const references */
  const static atools::fs::weather::MetarParser EMPTY;

  /* This ident is used in interpolated METAR strings */
  const static QLatin1String NO_IDENT;

private:
  bool scanPreambleDate();
  bool scanPreambleTime();
  void useCurrentDate();

  bool scanType();
  bool scanId();
  bool scanDate();
  bool scanModifier();
  bool scanWind();
  bool scanVariability();
  bool scanVisibility();
  bool scanRwyVisRange();
  bool scanSkyCondition();
  bool scanWeather();
  bool scanTemperature();
  bool scanPressure();
  bool scanRunwayReport();
  bool scanWindShear();
  bool scanTrendForecast();
  bool scanColorState();
  bool scanRemark();
  bool scanRemainder();

  int scanNumber(char **str, int *num, int min, int max = 0);
  bool scanBoundary(char **str);
  const struct Token *scanToken(char **str, const QVector<Token>& list);
  void normalizeData();

  /* Calculate flight rules (IFR, VFR, etc.), max and lowest ceiling*/
  void postProcess();
  void postProcessPrevailingWind();
  void postProcessFlightRules();
  void postProcessCloudCoverage();
  float lowestCloudBase() const;

  /* Build string from content */
  QString buildMetarString(const QStringList& remarks) const;

  QString getIntensityStringShort(int intensityValue, const QString& weatherPhenomenon) const;

  QString metar;
  QStringList errors;

  bool parsed = false;
  int groupCount;
  char *_data = nullptr;
  char *_m = nullptr;
  char _icao[5];
  int _year;
  int _month;
  int _day;
  int _hour;
  int _minute;
  int _report_type;
  int _wind_dir;
  float _wind_speed;
  float _gust_speed;
  int _wind_range_from;
  int _wind_range_to;
  float _temp;
  float _dewp;
  float _pressure;
  int _rain;
  int _hail;
  int _snow;
  bool _cavok;
  std::vector<struct Weather> _weather2;

  FlightRules flightRules = UNKNOWN;
  MetarCloud maxCoverageCloud, lowestCoverageCloud;
  int prevailingWindDir = -1;
  float prevailingWindSpeed = INVALID_METAR_VALUE;

  MetarVisibility _min_visibility;
  MetarVisibility _max_visibility;
  MetarVisibility _vert_visibility;
  MetarVisibility _dir_visibility[8];
  std::vector<MetarCloud> _clouds;
  std::map<std::string, MetarRunway> _runways;
  std::vector<std::string> _weather;
  std::string _remark, unusedData;

  bool fsxP3dFormat = false, incompleteInterpolation = false;
};

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_METAR_HXX
