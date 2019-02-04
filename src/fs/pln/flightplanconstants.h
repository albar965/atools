/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FLIGHTPLANCONSTANTS_H
#define ATOOLS_FLIGHTPLANCONSTANTS_H

#include <QLatin1Literal>

namespace atools {
namespace fs {
namespace pln {

enum SaveOption
{
  SAVE_NO_OPTIONS = 0,

  /* No XML comments (annotations) that can confuse incapable programs */
  SAVE_CLEAN = 1 << 1,

  /* Save Garmin GNS format with user waypoints */
  SAVE_GNS_USER_WAYPOINTS = 1 << 2
};

Q_DECLARE_FLAGS(SaveOptions, SaveOption);
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::pln::SaveOptions);

enum FileFormat
{
  NONE,
  PLN_FSX, // FSX or P3D XML PLN flight plan - can load and save
  PLN_FS9, // FS9 ini style PLN flight plan - can load only
  FMS3, // X-Plane version 3 FMS file - can load and save
  FMS11, // X-Plane version 11 FMS file - can load and save
  FLP, // Aerosoft airbus or FlightFactor Boeing - can load and save
  PLN_FSC, // FSC ini style PLN flight plan - can load only
  FLIGHTGEAR // FlightGear XML format - load and save
};

enum FlightplanType
{
  IFR,
  VFR
};

enum RouteType
{
  LOW_ALTITUDE,
  HIGH_ALTITUDE,
  VOR, /* Used for radio navaid routing (VOR and NDB) */
  DIRECT, /* Direct connection without waypoints */
  UNKNOWN /* Has to be changed later when resolving the ident to database objects */
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-const-variable"

/* Common key that are used int flight plan properties that are not supported in PLN.
 * Will be save inside a XML comment in pln files. */
/* Keys that describe procedures*/
const QLatin1Literal SIDAPPR("sidappr");
const QLatin1Literal SIDAPPRRW("sidapprrw");
const QLatin1Literal SIDAPPRDISTANCE("sidapprdistance");
const QLatin1Literal SIDAPPRSIZE("sidapprsize");

const QLatin1Literal SIDTRANS("sidtrans");
const QLatin1Literal SIDTRANSDISTANCE("sidtransdistance");
const QLatin1Literal SIDTRANSSIZE("sidtranssize");

const QLatin1Literal STAR("star");
const QLatin1Literal STARRW("starrw");
const QLatin1Literal STARDISTANCE("stardistance");
const QLatin1Literal STARSIZE("starsize");

const QLatin1Literal STARTRANS("startrans");
const QLatin1Literal STARTRANSDISTANCE("startransdistance");
const QLatin1Literal STARTRANSSIZE("startranssize");

const QLatin1Literal TRANSITION("transition");
const QLatin1Literal TRANSITIONTYPE("transitiontype");
const QLatin1Literal TRANSITIONDISTANCE("transitiondistance");
const QLatin1Literal TRANSITIONSIZE("transitionsize");

const QLatin1Literal APPROACH("approach");
const QLatin1Literal APPROACH_ARINC("approacharinc"); /* ARINC short name for FMS files */
const QLatin1Literal APPROACHTYPE("approachtype");
const QLatin1Literal APPROACHRW("approachrw");
const QLatin1Literal APPROACHSUFFIX("approachsuffix");
const QLatin1Literal APPROACHDISTANCE("approachdistance");
const QLatin1Literal APPROACHSIZE("approachsize");

/* Copies all related properties and deletes the ones in "to" that do not exist in "from". */
void copySidProcedureProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from);
void copyArrivalProcedureProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from);
void copyStarProcedureProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from);

/* Aircraft performance */
const QLatin1Literal AIRCRAFT_PERF_NAME("aircraftperfname");
const QLatin1Literal AIRCRAFT_PERF_TYPE("aircraftperftype");
const QLatin1Literal AIRCRAFT_PERF_FILE("aircraftperffile");

/* Source database navigation data */
const QLatin1Literal NAVDATA("navdata");

/* Source database simulator */
const QLatin1Literal SIMDATA("simdata");

/* AIRAC cycle (not FSX/P3D) */
const QLatin1Literal AIRAC_CYCLE("cycle");

/* Free parking spot name as not supported by PLN */
const QLatin1Literal PARKING("parking");

/* Position of parking as fallback */
const QLatin1Literal PARKINGPOS("parkingposition");

#pragma GCC diagnostic pop

} // namespace pln
} // namespace fs
} // namespace atools

#endif // ATOOLS_FLIGHTPLANCONSTANTS_H
