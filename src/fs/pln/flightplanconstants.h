/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

namespace pattern {

/* Pattern variables for flight plan names */
const QLatin1String PLANTYPE("PLANTYPE"); // IFR or VFR
const QLatin1String DEPARTIDENT("DEPARTIDENT"); // Departure airport ICAO code
const QLatin1String DEPARTNAME("DEPARTNAME"); // Departure airport name
const QLatin1String DESTIDENT("DESTIDENT"); // Destination airport ICAO code
const QLatin1String DESTNAME("DESTNAME"); // Destination airport name
const QLatin1String CRUISEALT("CRUISEALT"); // Cruise altitude

/* Default patterns */
const QString SHORT(DEPARTIDENT + " " + DESTIDENT);
const QString LONG(PLANTYPE + " " + DEPARTNAME + " (" + DEPARTIDENT + ") to " + DESTNAME + " (" + DESTIDENT + ")");
}

namespace entry {

enum WaypointType
{
  UNKNOWN,
  AIRPORT,
  WAYPOINT,
  VOR,
  NDB,
  USER
};

enum Flag
{
  NONE = 0,
  PROCEDURE = 1 << 1, /* Flight plan entry is any procedure leg */
  ALTERNATE = 1 << 2, /* Flight plan entry leads to an alternate airport */
  TRACK = 1 << 3 /* Flight plan entry airway name is a track */
};

Q_DECLARE_FLAGS(Flags, Flag);
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::pln::entry::Flags);
}

/* File format for flight plans as detected by FlightplanIO::load and detectFormat */
enum FileFormat
{
  NONE,
  FSX_PLN, /* FSX or P3D XML PLN flight plan - can load and save */
  FS9_PLN, /* FS9 ini style PLN flight plan - can load only */
  FMS3, /* X-Plane version 3 FMS file - can load and save */
  FMS11, /* X-Plane version 11 FMS file - can load and save */
  FLP, /* Aerosoft airbus or FlightFactor Boeing - can load and save */
  FSC_PLN, /* FSC ini style PLN flight plan - can load only */
  FLIGHTGEAR, /* FlightGear XML format - load and save  */
  LNM_PLN, /* Little Navmap's own flight plan format  */
  GARMIN_FPL, /* Garmin FPL - XML format */
  MSFS_PLN /* MSFS PLN  */
};

enum FlightplanType
{
  IFR,
  VFR
};

/* Currently only used for saving FSX/P3D PLN files */
enum RouteType
{
  LOW_ALTITUDE,
  HIGH_ALTITUDE,
  VOR, /* Used for radio navaid routing (VOR and NDB) */
  DIRECT, /* Direct connection without waypoints */
  UNKNOWN /* Has to be changed later when resolving the ident to database objects */
};

/* Common key that are used int flight plan properties that are not supported in PLN.
 * Will be save inside a XML comment in pln files. */
/* Keys that describe procedures*/
const QLatin1Literal SIDAPPR("sidappr");
const QLatin1Literal SIDAPPRRW("sidapprrw");
const QLatin1Literal SIDTRANS("sidtrans");

const QLatin1Literal STAR("star");
const QLatin1Literal STARRW("starrw");
const QLatin1Literal STARTRANS("startrans");

const QLatin1Literal TRANSITION("transition");
const QLatin1Literal TRANSITIONTYPE("transitiontype");

const QLatin1Literal APPROACH("approach");
const QLatin1Literal APPROACH_ARINC("approacharinc"); /* ARINC short name for FMS files */
const QLatin1Literal APPROACHTYPE("approachtype");
const QLatin1Literal APPROACHRW("approachrw");
const QLatin1Literal APPROACHSUFFIX("approachsuffix");

/* Name of airway which leads to IAF of a STAR or an approach (-transition) */
const QLatin1Literal PROCAIRWAY("procairway");

/* Only for approachtype = CUSTOM */
const QLatin1Literal APPROACH_CUSTOM_DISTANCE("approachcustomdistance");
const QLatin1Literal APPROACH_CUSTOM_ALTITUDE("approachcustomaltitude");

/* List of alternate airport(s) separated by "#" */
const QLatin1Literal ALTERNATES("alternates");

/* Copies all related properties and deletes the ones in "to" that do not exist in "from". */
void copySidProcedureProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from);
void copyArrivalProcedureProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from);
void copyStarProcedureProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from);
void copyAlternateProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from);

/* Aircraft performance */
const QLatin1Literal AIRCRAFT_PERF_NAME("aircraftperfname");
const QLatin1Literal AIRCRAFT_PERF_TYPE("aircraftperftype");
const QLatin1Literal AIRCRAFT_PERF_FILE("aircraftperffile");

/* Source database navigation data */
const QLatin1Literal NAVDATA("navdata");
const QLatin1Literal NAVDATACYCLE("navdatacycle");

/* Source database simulator */
const QLatin1Literal SIMDATA("simdata");
const QLatin1Literal SIMDATACYCLE("simdatacycle");

} // namespace pln
} // namespace fs
} // namespace atools

#endif // ATOOLS_FLIGHTPLANCONSTANTS_H
