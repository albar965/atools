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

#ifndef ATOOLS_SC_TYPES_H
#define ATOOLS_SC_TYPES_H

#include "util/flags.h"

#include <QVector>
#include <QObject>
#include <QDateTime>

namespace atools {
namespace fs {
namespace sc {

const float SC_INVALID_FLOAT = std::numeric_limits<float>::max();
const float SC_INVALID_QUINT16 = std::numeric_limits<quint16>::max();
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

Q_DECLARE_FLAGS(Options, Option)
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::sc::Options)

enum AircraftFlag : quint16
{
  NONE = 0x0000,
  ON_GROUND = 0x0001,
  IN_CLOUD = 0x0002,
  IN_RAIN = 0x0004,
  IN_SNOW = 0x0008,
  IS_USER = 0x0010,

  /* Indicated source simulator for all aircraft */
  SIM_FSX_P3D = 0x0020,
  SIM_XPLANE11 = 0x0040,
  SIM_XPLANE12 = 0x1000,
  SIM_MSFS = 0x0800,

  /* Simulator is in pause mode */
  SIM_PAUSED = 0x0080,

  /* Replay is active - only X-Plane */
  SIM_REPLAY = 0x0100,

  /* Built from online network data - not in simulator */
  SIM_ONLINE = 0x0200,

  /* A simulator aircraft also recognized as online network aircraft.
   * This simulator aircraft is a shadow of an online network aircraft (by matching callsign to reg or position/altitude).
   * Set in OnlinedataController::updateAircraftShadowState() in the client (LNM) */
  SIM_ONLINE_SHADOW = 0x0400

                      // Next is 0x2000
};

ATOOLS_DECLARE_FLAGS_16(AircraftFlags, AircraftFlag)
ATOOLS_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::sc::AircraftFlags)

void registerMetaTypes();

} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_METATYPE(atools::fs::sc::SimConnectStatus)

#endif // ATOOLS_SC_TYPES_H
