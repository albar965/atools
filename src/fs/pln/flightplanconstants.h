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

#ifndef ATOOLS_FLIGHTPLANCONSTANTS_H
#define ATOOLS_FLIGHTPLANCONSTANTS_H

#include <limits>

#include <QLatin1String>

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
  MSFS_PLN, /* MSFS PLN  */
  GARMIN_GFP /* Garmin GFP - One line text format prefixed with "FPN/RI:..." */
};

enum FlightplanType
{
  NO_TYPE,
  IFR,
  VFR
};

/* Departure parking type. Describes content of FlightPlan::departureParkingName */
enum FlightplanParkingType
{
  NO_POS,
  AIRPORT,
  RUNWAY, /* Zero prefixed runway number like "07C" */
  PARKING, /* Free X-Plane parking name or FSX/P3D/MSFS naming scheme. */
  HELIPAD /* Number describing the helipad */
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

/*
 * Common key that are used int flight plan properties that are not supported in PLN.
 * Used to transport more optional details to the export methods.
 * Keys that describe procedures */

/* SID, runway and transiton */
const QLatin1String SID("sidappr"); /* SID name */
const QLatin1String SIDRW("sidapprrw"); /* SID runway */
const QLatin1String SIDTRANS("sidtrans"); /* SID transition name */
const QLatin1String SIDTRANSWP("sidtranswp"); /* Alternative to above, the endpoint of the SID transition */
const QLatin1String SIDTYPE("sidtype"); /* Optional type of SID, CUSTOMDEPART for selected runway */

/* STAR, runway and transiton */
const QLatin1String STAR("star"); /* STAR name */
const QLatin1String STARRW("starrw"); /* STAR runway */
const QLatin1String STARTRANS("startrans"); /* STAR transition name */
const QLatin1String STARTRANSWP("startranswp"); /* Alternative to above, the possible startpoints of a STAR transition
                                                 *  separated by PROPERTY_LIST_SEP */

/* Approach, runway and more */
const QLatin1String APPROACH("approach"); /* Approach name like waypoint */
const QLatin1String APPROACH_ARINC("approacharinc"); /* ARINC short name for FMS files */
const QLatin1String APPROACHTYPE("approachtype"); /* Optional type like ILS, RNAV, etc. CUSTOM for selected arrival runway and leg distances. */
const QLatin1String APPROACHRW("approachrw"); /* Runway for approach */
const QLatin1String APPROACHSUFFIX("approachsuffix"); /* Suffix like Z for ILS-Z */

/* Approach transiton */
const QLatin1String TRANSITION("transition"); /* Transition name */
const QLatin1String TRANSITIONTYPE("transitiontype"); /* Type: Full, etc. */

/* Name of airway which leads to IAF of a STAR or an approach (-transition) */
const QLatin1String PROCAIRWAY("procairway");

/* Only for APPROACHTYPE = CUSTOM */
const QLatin1String APPROACH_CUSTOM_DISTANCE("approachcustomdistance"); /* Length or final leg in NM */
const QLatin1String APPROACH_CUSTOM_ALTITUDE("approachcustomaltitude"); /* Entry altitude at final leg in ft */
const QLatin1String APPROACH_CUSTOM_OFFSET("approachcustomoffset"); /* Offset angle for approach */

/* Only for SIDTYPE = CUSTOMDEPART */
const QLatin1String DEPARTURE_CUSTOM_DISTANCE("departurecustomdistance"); /* Length or final leg in NM */

/* List of alternate airport(s) separated by PROPERTY_LIST_SEP */
const QLatin1String ALTERNATES("alternates");

/* Separator character for alternate and transition waypoint lists */
const QLatin1Char PROPERTY_LIST_SEP('#');

/* Copies all related properties and deletes the ones in "to" that do not exist in "from". */
void copySidProcedureProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from);
void copyArrivalProcedureProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from);
void copyStarProcedureProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from);
void copyAlternateProperties(QHash<QString, QString>& to, const QHash<QString, QString>& from);

/* Aircraft performance */
const QLatin1String AIRCRAFT_PERF_NAME("aircraftperfname");
const QLatin1String AIRCRAFT_PERF_TYPE("aircraftperftype");
const QLatin1String AIRCRAFT_PERF_FILE("aircraftperffile");

/* Source database navigation data */
const QLatin1String NAVDATA("navdata");
const QLatin1String NAVDATACYCLE("navdatacycle");

/* Source database simulator */
const QLatin1String SIMDATA("simdata");
const QLatin1String SIMDATACYCLE("simdatacycle");

/* Set if any of the airports to use DEP/DES instead of ADES/ADEP for loading in X-Plane FMS.
 * Read by FlightplanIO::saveFmsInternal() */
const QLatin1String AIRPORT_DEPARTURE_NO_AIRPORT("departnoapt");
const QLatin1String AIRPORT_DESTINATION_NO_AIRPORT("destnoapt");

/* Invalid/not set heading for parking position */
const float INVALID_HEADING = std::numeric_limits<float>::max();

/* Invalid position altitude for parking and departure */
const float INVALID_ALTITUDE = std::numeric_limits<float>::max();

} // namespace pln
} // namespace fs
} // namespace atools

#endif // ATOOLS_FLIGHTPLANCONSTANTS_H
