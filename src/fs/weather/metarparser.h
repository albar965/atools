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
    distance(INVALID_METAR_VALUE), direction(-1), modifier(EQUALS), tendency(NONE)
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

  void set(float dist, int dir = -1, int mod = -1, int tend = -1);

  inline float getVisibilityMeter() const
  {
    return distance;
  }

  inline int getDirection() const
  {
    return direction;
  }

  inline int getModifier() const
  {
    return modifier;
  }

  QString getModifierString() const;

  inline int getTendency() const
  {
    return tendency;
  }

private:
  float distance;
  int direction;
  int modifier;
  int tendency;
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

  inline int getDeposit() const
  {
    return deposit;
  }

  inline QString getDepositString() const
  {
    return depositString;
  }

  inline float getExtent() const
  {
    return extent;
  }

  inline QString getExtentString() const
  {
    return extentString;
  }

  inline float getDepth() const
  {
    return depth;
  }

  inline float getFriction() const
  {
    return friction;
  }

  inline QString getFrictionString() const
  {
    return frictionString;
  }

  inline QString getComment() const
  {
    return comment;
  }

  inline bool getWindShear() const
  {
    return windShear;
  }

  inline const MetarVisibility& getMinVisibility() const
  {
    return minVisibility;
  }

  inline const MetarVisibility& getMaxVisibility() const
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

  MetarCloud() :
    coverage(COVERAGE_NIL), altitude(INVALID_METAR_VALUE)
  {
  }

  void set(float alt, Coverage cov = COVERAGE_NIL);

  inline Coverage getCoverage() const
  {
    return coverage;
  }

  static QString getCoverageString(Coverage cloudCoverage);
  QString getCoverageString() const;

  static Coverage getCoverage(const QString& coverage);

  inline float getAltitudeMeter() const
  {
    return altitude;
  }

  inline QString getTypeString() const
  {
    return type;
  }

  inline QString getTypeLongString() const
  {
    return typeLong;
  }

private:
  Coverage coverage; // quarters: 0 -> clear ... 4 -> overcast
  float altitude; // 1000 m
  QString type; // CU
  QString typeLong; // cumulus
};

// ============================================================================
// ============================================================================
class MetarParser
{
  Q_DECLARE_TR_FUNCTIONS(MetarParser)

public:
  explicit MetarParser(const QString& metar);
  ~MetarParser();

  static QDateTime extractDateTime(const QString& metar);

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

  inline QString getUnusedData() const
  {
    return QString::fromStdString(unusedData);
  }

  inline bool getProxy() const
  {
    return _x_proxy;
  }

  inline QString getId() const
  {
    return _icao;
  }

  inline int getYear() const
  {
    return _year;
  }

  inline int getMonth() const
  {
    return _month;
  }

  inline int getDay() const
  {
    return _day;
  }

  inline int getHour() const
  {
    return _hour;
  }

  inline int getMinute() const
  {
    return _minute;
  }

  QDateTime getDateTime() const;

  inline int getReportType() const
  {
    return _report_type;
  }

  QString getReportTypeString() const;

  inline int getWindDir() const
  {
    return _wind_dir;
  }

  inline float getWindSpeedMeterPerSec() const
  {
    return _wind_speed;
  }

  float getWindSpeedKts() const;

  inline float getGustSpeedMeterPerSec() const
  {
    return _gust_speed;
  }

  float getGustSpeedKts() const;

  inline int getWindRangeFrom() const
  {
    return _wind_range_from;
  }

  inline int getWindRangeTo() const
  {
    return _wind_range_to;
  }

  inline const MetarVisibility& getMinVisibility() const
  {
    return _min_visibility;
  }

  inline const MetarVisibility& getMaxVisibility() const
  {
    return _max_visibility;
  }

  inline const MetarVisibility& getVertVisibility() const
  {
    return _vert_visibility;
  }

  inline const MetarVisibility *getDirVisibility() const
  {
    return _dir_visibility;
  }

  inline float getTemperatureC() const
  {
    return _temp;
  }

  inline float getDewpointDegC() const
  {
    return _dewp;
  }

  inline float getPressureMbar() const
  {
    return _pressure < INVALID_METAR_VALUE ? _pressure / 100 : INVALID_METAR_VALUE;
  }

  inline int getRain() const
  {
    return _rain;
  }

  inline int getHail() const
  {
    return _hail;
  }

  inline int getSnow() const
  {
    return _snow;
  }

  QString getIntensityString(int intensity) const;

  inline bool getCavok() const
  {
    return _cavok;
  }

  double getRelHumidity() const;

  inline QList<MetarCloud> getClouds() const
  {
    return QList<MetarCloud>(_clouds.begin(), _clouds.end());
  }

  inline QHash<QString, MetarRunway> getRunways() const;

  QStringList getWeather() const;

  inline QList<Weather> getWeather2() const
  {
    return QList<Weather>(_weather2.begin(), _weather2.end());
  }

  bool isValid() const
  {
    return valid;
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
    return maxCoverage;
  }

  QString getMaxCoverageString() const
  {
    return MetarCloud::getCoverageString(maxCoverage);
  }

  /* Coverage of lowest cloud layer */
  MetarCloud::Coverage getLowestCoverage() const
  {
    return lowestCoverage;
  }

  QString getLowestCoverageString() const
  {
    return MetarCloud::getCoverageString(lowestCoverage);
  }

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

  bool valid = false;
  std::string _url;
  int _grpcount;
  bool _x_proxy;
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
  MetarCloud::Coverage maxCoverage = MetarCloud::COVERAGE_CLEAR;
  MetarCloud::Coverage lowestCoverage = MetarCloud::COVERAGE_CLEAR;
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

};

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_METAR_HXX
