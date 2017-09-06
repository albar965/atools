/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_SC_TYPES_H
#define ATOOLS_SC_TYPES_H

#include "geo/pos.h"

#include <QVector>
#include <QObject>
#include <QDateTime>

namespace atools {
namespace fs {
namespace sc {

const float SC_INVALID_FLOAT = std::numeric_limits<float>::max();
const int SC_INVALID_INT = std::numeric_limits<int>::max();

enum SimConnectStatus
{
  OK, /* No error */
  INVALID_MAGIC_NUMBER, /* Packet data does not start with expected magic number */
  VERSION_MISMATCH, /* Client and server data version does not match for either data or reply */
  INSUFFICIENT_WRITE, /* Wrote less than block */
  WRITE_ERROR /* Error from IO device */
};

enum Option
{
  NO_OPTION = 0,
  FETCH_AI_AIRCRAFT = 1 << 0,
  FETCH_AI_BOAT = 1 << 1
};

Q_DECLARE_FLAGS(Options, Option);
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::sc::Options);

// quint16
enum AircraftFlag
{
  NONE = 0x0000,
  ON_GROUND = 0x0001,
  IN_CLOUD = 0x0002,
  IN_RAIN = 0x0004,
  IN_SNOW = 0x0008,
  IS_USER = 0x0010,

  /* Indicated source simulator for all aircraft */
  SIM_FSX_P3D = 0x0020,
  SIM_XPLANE = 0x0040
};

Q_DECLARE_FLAGS(AircraftFlags, AircraftFlag);
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::sc::AircraftFlags);

const QVector<QString> SIMCONNECT_STATUS_TEXT =
{
  QObject::tr("No Error"),
  QObject::tr("Invalid magic number"),
  QObject::tr("Version mismatch"),
  QObject::tr("Incomplete write"),
  QObject::tr("Write error")
};

struct MetarResult
{
  QString requestIdent, metarForStation, metarForNearest, metarForInterpolated;
  atools::geo::Pos requestPos;
  QDateTime timestamp;

  bool isValid() const
  {
    return !requestIdent.isEmpty() && requestPos.isValid();
  }

  bool isEmpty() const
  {
    return !isValid() ||
           (metarForStation.isEmpty() && metarForNearest.isEmpty() && metarForInterpolated.isEmpty());
  }

  bool operator==(const atools::fs::sc::MetarResult& other)
  {
    return requestIdent == other.requestIdent &&
           metarForStation == other.metarForStation &&
           metarForNearest == other.metarForNearest &&
           metarForInterpolated == other.metarForInterpolated &&
           requestPos == other.requestPos;
  }

  bool operator!=(const atools::fs::sc::MetarResult& other)
  {
    return !operator==(other);
  }

};

} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_SC_TYPES_H
