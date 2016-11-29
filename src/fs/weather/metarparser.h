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

#ifndef _METAR_HXX
#define _METAR_HXX

#include "atools.h"
#include "geo/calculations.h"

#include <QObject>
#include <vector>
#include <map>
#include <string>
#include <QVector>

// #include <simgear/constants.h>
namespace atools {
namespace fs {
namespace weather {

struct Token
{
  const char *id;
  const char *text;
};

const float MetarNaN = -1E20f;

class MetarParser;

// ============================================================================
// ============================================================================
class MetarVisibility
{
  friend class MetarParser;

public:
  MetarVisibility() :
    _distance(MetarNaN), _direction(-1), _modifier(EQUALS), _tendency(NONE)
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
    return _distance;
  }

  inline int getDirection() const
  {
    return _direction;
  }

  inline int getModifier() const
  {
    return _modifier;
  }

  QString getModifierString() const;

  inline int getTendency() const
  {
    return _tendency;
  }

protected:
  float _distance;
  int _direction;
  int _modifier;
  int _tendency;
};

// ============================================================================
// ============================================================================
// runway condition (surface and visibility)
class MetarRunway
{
  friend class MetarParser;

public:
  MetarRunway() :
    _deposit(-1), _deposit_string(0), _extent(-1), _extent_string(0), _depth(MetarNaN), _friction(MetarNaN),
    _friction_string(0), _comment(0), _wind_shear(false)
  {
  }

  inline int getDeposit() const
  {
    return _deposit;
  }

  inline QString getDepositString() const
  {
    return _deposit_string;
  }

  inline float getExtent() const
  {
    return _extent;
  }

  inline QString getExtentString() const
  {
    return _extent_string;
  }

  inline float getDepth() const
  {
    return _depth;
  }

  inline float getFriction() const
  {
    return _friction;
  }

  inline QString getFrictionString() const
  {
    return _friction_string;
  }

  inline QString getComment() const
  {
    return _comment;
  }

  inline bool getWindShear() const
  {
    return _wind_shear;
  }

  inline const MetarVisibility& getMinVisibility() const
  {
    return _min_visibility;
  }

  inline const MetarVisibility& getMaxVisibility() const
  {
    return _max_visibility;
  }

protected:
  MetarVisibility _min_visibility;
  MetarVisibility _max_visibility;
  int _deposit;
  const char *_deposit_string;
  int _extent;
  const char *_extent_string;
  float _depth;
  float _friction;
  const char *_friction_string;
  const char *_comment;
  bool _wind_shear;
};

// ============================================================================
// ============================================================================
// cloud layer
class MetarCloud
{
  friend class MetarParser;

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

  static const char *COVERAGE_NIL_STRING;
  static const char *COVERAGE_CLEAR_STRING;
  static const char *COVERAGE_FEW_STRING;
  static const char *COVERAGE_SCATTERED_STRING;
  static const char *COVERAGE_BROKEN_STRING;
  static const char *COVERAGE_OVERCAST_STRING;

  MetarCloud() :
    _coverage(COVERAGE_NIL), _altitude(MetarNaN), _type(0), _type_long(0)
  {
  }

  void set(float alt, Coverage cov = COVERAGE_NIL);

  inline Coverage getCoverage() const
  {
    return _coverage;
  }

  QString getCoverageString() const;

  static Coverage getCoverage(const QString& coverage);

  inline float getAltitudeMeter() const
  {
    return _altitude;
  }

  inline QString getTypeString() const
  {
    return _type;
  }

  inline QString getTypeLongString() const
  {
    return _type_long;
  }

protected:
  Coverage _coverage; // quarters: 0 -> clear ... 4 -> overcast
  float _altitude; // 1000 m
  const char *_type; // CU
  const char *_type_long; // cumulus
};

// ============================================================================
// ============================================================================
class MetarParser
{
public:
  MetarParser(const QString& metar);
  ~MetarParser();

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

  inline QString getData() const
  {
    return _data;
  }

  inline QString getUnusedData() const
  {
    return _m;
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

  inline float getGustSpeedMeterPerSec() const
  {
    return _gust_speed;
  }

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
    return _pressure == MetarNaN ? MetarNaN : _pressure / 100;
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

  inline QVector<MetarCloud> getClouds() const
  {
    return QVector<MetarCloud>::fromStdVector(_clouds);
  }

  inline QHash<QString, MetarRunway> getRunways() const;

  QStringList getWeather() const;

  inline QVector<struct Weather> getWeather2() const
  {
    return QVector<struct Weather>::fromStdVector(_weather2);

  }

  bool isValid() const
  {
    return _data != nullptr;
  }

  QString getRemark() const
  {
    return QString::fromStdString(_remark);
  }

protected:
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

  MetarVisibility _min_visibility;
  MetarVisibility _max_visibility;
  MetarVisibility _vert_visibility;
  MetarVisibility _dir_visibility[8];
  std::vector<MetarCloud> _clouds;
  std::map<std::string, MetarRunway> _runways;
  std::vector<std::string> _weather;
  std::string _remark;

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
  const struct Token *scanToken(char **str, const struct Token *list);
  void normalizeData();

};

} // namespace weather
} // namespace fs
} // namespace atools

#endif // _METAR_HXX
