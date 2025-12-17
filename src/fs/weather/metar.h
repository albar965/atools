/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_METAR_H
#define ATOOLS_METAR_H

#include "fs/weather/weathertypes.h"
#include "fs/weather/metarparser.h"
#include "geo/linestring.h"

#include <QDateTime>

namespace atools {
namespace fs {
namespace weather {

class Metar;
typedef QList<Metar> MetarList;
typedef QList<const Metar *> MetarPtrList;

/*
 * Collects METAR information for station, nearest and interpolated values.
 * Also keeps position and ident of original request.
 */
class Metar
{
public:
  Metar()
  {
  }

  /* Interpolates weather between list "metars". Interpolates, wind, flight rules, visibility and pressure but not clouds. */
  Metar(const QString& requestIdentParam, const atools::geo::Pos& posParam, const MetarPtrList& metars);

  /* METAR using given time */
  Metar(const QString& requestIdentParam, const atools::geo::Pos& posParam, const QDateTime& timestampParam, const QString& metarString)
    : requestIdent(requestIdentParam), requestPos(posParam), timestamp(timestampParam), station(metarString)
  {
  }

  /* METAR using current time */
  Metar(const QString& requestIdentParam, const atools::geo::Pos& posParam, const QString& metarString = QString())
    : requestIdent(requestIdentParam), requestPos(posParam), timestamp(QDateTime::currentDateTimeUtc()), station(metarString)
  {

  }

  /* Decode the given METAR string and fill all fields. useTimestamp: Copy best (station, nearest and interpolated) parsed timestamp
   *  into useTimestamp */
  void parseAll(bool useTimestamp);

  /* Convert FSX and P3D special METARs to normal METARs */
  void cleanFsxP3dAll();

  /* Resets parsed values and clears METAR string */
  void clearAll();

  /* Weather at exact station with given ident */
  const atools::fs::weather::MetarParser& getMetarParserStation() const
  {
    return getMetarParser(atools::fs::weather::STATION);
  }

  /* Interpolated weeather */
  const atools::fs::weather::MetarParser& getMetarParserInterpolated() const
  {
    return getMetarParser(atools::fs::weather::INTERPOLATED);
  }

  /* Weather at the nearest station */
  const atools::fs::weather::MetarParser& getMetarParserNearest() const
  {
    return getMetarParser(atools::fs::weather::NEAREST);
  }

  /* Get parsed METAR for given type */
  const atools::fs::weather::MetarParser& getMetarParser(atools::fs::weather::MetarType type) const;

  bool hasAnyMetar() const
  {
    return station.hasMetarString() || interpolated.hasMetarString() || nearest.hasMetarString();
  }

  bool hasStationMetar() const
  {
    return station.hasMetarString();
  }

  bool operator==(const atools::fs::weather::Metar& other)
  {
    return requestIdent == other.requestIdent &&
           requestPos.almostEqual(other.requestPos) &&
           station.getMetarString() == other.station.getMetarString() &&
           nearest.getMetarString() == other.nearest.getMetarString() &&
           interpolated.getMetarString() == other.interpolated.getMetarString();
  }

  bool operator!=(const atools::fs::weather::Metar& other)
  {
    return !operator==(other);
  }

  /* Indent passed in for request. Will be different from idents in nearest and interpolated. */
  const QString& getRequestIdent() const
  {
    return requestIdent;
  }

  /* Position when requested */
  const atools::geo::Pos& getRequestPos() const
  {
    return requestPos;
  }

  /* Airport idents used for interpolation */
  const QStringList& getInterpolatedIdents() const
  {
    return interpolatedIdents;
  }

  const atools::geo::LineString& getInterpolatedPositions() const
  {
    return interpolatedPositions;
  }

  /* Airport ident for nearest */
  const QString getNearestIdent() const
  {
    return nearest.getId();
  }

  const QString& getStationMetar() const
  {
    return station.getMetarString();
  }

  const QString& getInterpolatedMetar() const
  {
    return interpolated.getMetarString();
  }

  const QString& getNearestMetar() const
  {
    return nearest.getMetarString();
  }

  const atools::fs::weather::MetarParser& getStation() const
  {
    return station;
  }

  const atools::fs::weather::MetarParser& getInterpolated() const
  {
    return interpolated;
  }

  const atools::fs::weather::MetarParser& getNearest() const
  {
    return nearest;
  }

  /* Timestamp as passed in from the constructor */
  const QDateTime& getTimestamp() const
  {
    return timestamp;
  }

  /* Position as passed in from the constructor */
  const atools::geo::Pos& getPosition() const
  {
    return requestPos;
  }

  /* METARs are in special FSX/P3D format */
  bool isFsxP3dFormat() const
  {
    return station.isFsxP3dFormat() || interpolated.isFsxP3dFormat() || nearest.isFsxP3dFormat();
  }

  /* Interpolation is omitted if airport is closer. Only nearest is returned. */
  void setMinInterpolationDistMeter(float value)
  {
    minInterpolationDistMeter = value;
  }

  // Setters used internally ========================================================================
  void setRequestIdent(const QString& value)
  {
    requestIdent = value;
  }

  void setMetarForStation(const QString& value)
  {
    station.setMetar(value);
  }

  void setRequestPos(const atools::geo::Pos& value)
  {
    requestPos = value;
  }

  void setTimestamp(const QDateTime& value)
  {
    timestamp = value;
  }

  void setMetarForInterpolated(const QString& metar)
  {
    interpolated.setMetar(metar);
  }

  void setMetarForNearest(const QString& metar)
  {
    nearest.setMetar(metar);
  }

  /* Format has to be converted from FSX/P3D */
  void setFsxP3dFormat(bool value = true)
  {
    station.setFsxP3dFormat(value);
    interpolated.setFsxP3dFormat(value);
    nearest.setFsxP3dFormat(value);
  }

  /* Empty instance can be used when returned as reference */
  const static atools::fs::weather::Metar EMPTY;

private:
  friend QDebug operator<<(QDebug out, const Metar& record);

  QString cleanMetar(const QString& metar);

  QString requestIdent;
  atools::geo::Pos requestPos;
  QDateTime timestamp;
  atools::fs::weather::MetarParser station, interpolated, nearest;
  QStringList interpolatedIdents;
  atools::geo::LineString interpolatedPositions;

  float minInterpolationDistMeter = 100.f;
};

QDebug operator<<(QDebug out, const atools::fs::weather::Metar& metar);

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_METAR_H
