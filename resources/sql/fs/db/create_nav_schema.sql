-- *****************************************************************************
-- Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.
-- ****************************************************************************/

-- *************************************************************
-- This script create all tables used for navigation aids
-- *************************************************************

drop table if exists waypoint;

-- Waypoint/intersection navaid
create table waypoint
(
  waypoint_id integer primary key,
  file_id integer not null,
  nav_id integer,                     -- Refers to vor.vor_id or ndb.ndb_id depending on type
  ident varchar(5) not null,          -- ICAO ident
  region varchar(2),                  -- ICAO two letter region identifier
  airport_id integer,                 -- Reference to airport if applicable
  airport_ident varchar(4),           -- Redundant to id - used for resolving id after compilation
  artificial integer,                 -- Created for VOR and NDB navaids to support airway generation.
                                      -- 1 = for airways and 2 = for procedures (2 is only needed for LNM <= 2.4.5)
  type varchar(15),                   -- see enum atools::fs::bgl::nav::WaypointType
                                      -- N = NDB, OA = off airway, V = VOR, WN = named waypoint, WU = unnamed waypoint
  arinc_type varchar(4),              -- ARINC Waypoint type as defined by the 3 columns of ARINC 424.18 field definition 5.42
  num_victor_airway integer not null, -- Number of victor (low altitude) airways crossing this waypoint
  num_jet_airway integer not null,    -- Number of jet (high altitude) airways crossing this waypoint
  mag_var double not null,            -- Magnetic variance in degree < 0 for West and > 0 for East
  lonx double not null,
  laty double not null,
foreign key(file_id) references bgl_file(bgl_file_id),
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_waypoint_file_id on waypoint(file_id);
create index if not exists idx_waypoint_airport_id on waypoint(airport_id);
create index if not exists idx_waypoint_nav_id on waypoint(nav_id);

-- **************************************************

drop table if exists vor;

-- VHF omnidirectional range - VOR, VORDME, DME or TACAN
create table vor
(
  vor_id integer primary key,
  file_id integer not null,
  ident varchar(5),             -- ICAO ident
  name varchar(50),
  region varchar(2),            -- ICAO two letter region identifier
  airport_id integer,           -- Reference to airport if applicable
  airport_ident varchar(4),     -- Redundant to id - used for resolving id after compilation
  type varchar(15),             -- See enum atools::fs::bgl::nav::IlsVorType
                                --  H = high VOR, L = low VOR, T = terminal VOR,
                                -- TC = TACAN, VTH = high VORTAC, VTL = low VORTAC, VTT = terminal VORTAC
  frequency integer,            -- Frequency - MHz * 1000
  channel varchar(5),           -- TACAN channel
  range integer,                -- Navaid radio range in NM
  mag_var double,               -- Magnetic variance in degree < 0 for West and > 0 for East
                                -- Will be calculated if null
  dme_only integer not null,    -- 1 if this is only a DME
  dme_altitude integer,         -- Feet - null if navaid is VOR only
  dme_lonx double,              -- Null if navaid is VOR only
  dme_laty double,              -- "
  altitude integer,             -- Feet or null if not available
  lonx double not null,
  laty double not null,
foreign key(file_id) references bgl_file(bgl_file_id),
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_vor_file_id on vor(file_id);
create index if not exists idx_vor_airport_id on vor(airport_id);

-- **************************************************

drop table if exists ndb;

-- Non directional beacons
create table ndb
(
  ndb_id integer primary key,
  file_id integer not null,
  ident varchar(5),           -- ICAO ident
  name varchar(50),
  region varchar(2),          -- ICAO two letter region identifier
  airport_id integer,         -- Reference to airport if applicable
  airport_ident varchar(4),   -- Redundant to id - used for resolving id after compilation
  type varchar(15),           -- See enum atools::fs::bgl::nav::NdbType
                              -- CP MH H HH
  frequency integer not null, -- Frequency - kHz * 100
  range integer,              -- Navaid radio range in NM or null if not available
  mag_var double  not null,   -- Magnetic variance in degree < 0 for West and > 0 for East
  altitude integer,           -- Feet
  lonx double not null,
  laty double not null,
foreign key(file_id) references bgl_file(bgl_file_id),
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_ndb_file_id on ndb(file_id);
create index if not exists idx_ndb_airport_id on ndb(airport_id);

-- **************************************************

drop table if exists marker;

-- Approach marker
create table marker
(
  marker_id integer primary key,
  file_id integer not null,
  ident varchar(5),              -- ICAO ident
  region varchar(2),             -- ICAO two letter region identifier (always null)
  type varchar(15),              -- See enum atools::fs::bgl::nav::MarkerType
  heading double not null,       -- Heading in degree true
  altitude integer not null,     -- Feet
  lonx double not null,
  laty double not null,
foreign key(file_id) references bgl_file(bgl_file_id)
);

create index if not exists idx_marker_file_id on marker(file_id);

-- **************************************************

drop table if exists ils;

-- Instrument landing system and SBAS/GBAS path points as well as stations
create table ils
(
  ils_id integer primary key,
  ident varchar(5),                -- ICAO ident
  name varchar(50),
  region varchar(2),               -- ICAO two letter region identifier (always null)

  type varchar(1),             -- null, unknown
                                   -- ILS Localizer only, no glideslope   0
                                   -- ILS Localizer/MLS/GLS Unknown cat   U
                                   -- ILS Localizer/MLS/GLS Cat I         1
                                   -- ILS Localizer/MLS/GLS Cat II        2
                                   -- ILS Localizer/MLS/GLS Cat III       3
                                   -- IGS Facility                        I
                                   -- LDA Facility with glideslope        L
                                   -- LDA Facility no glideslope          A
                                   -- SDF Facility with glideslope        S
                                   -- SDF Facility no glideslope          F
                                   -- GLS ground station:                 G
                                   -- GLS / GBAS threshold point:         T

  perf_indicator varchar(10),      -- "LP", "LPV", "APV-II" and "GLS"
  provider varchar(10),            -- Provider of the SBAS service can be "WAAS", "EGNOS", "MSAS".
                                   -- If no provider is specified, or this belongs to a GLS approach, then "GP"


  frequency integer,               -- MHz * 1000 or GLS channel
  range integer,                   -- Range in NM (currently always 27)
  mag_var double  not null,        -- Magnetic variance in degree < 0 for West and > 0 for East
  has_backcourse integer not null, -- 1 if backcourse is available
  dme_range integer,               -- Range of DME if available
  dme_altitude integer,            -- Feet if available - otherwise null
  dme_lonx double,
  dme_laty double,
  gs_range integer,                -- Glideslope range in NM if available - otherwise null
  gs_pitch double,                 -- Glideslope pitch in degree or null if not available
  gs_altitude integer,             -- Glideslope altitude - feet or null if not available
  gs_lonx double,
  gs_laty double,
  loc_runway_end_id integer,       -- Reference to runway end.
  loc_airport_ident varchar(4),
  loc_runway_name varchar(10),
  loc_heading double,              -- Localizer heading in degree true
  loc_width double,                -- Width of localizer in degree or null if not available
  end1_lonx double,                -- Pre-calculated first endpoint for a 8 NM feather
  end1_laty double,                -- "
  end_mid_lonx double,             -- Pre-calculated endpoint in the center of the end
  end_mid_laty double,             -- "
  end2_lonx double,                -- Pre-calculated second endpoint for a 8 NM feather
  end2_laty double,                -- "
  altitude integer not null,       -- Feet
  lonx double not null,            -- Coordinates of the ILS origin
  laty double not null,            -- "
foreign key(loc_runway_end_id) references runway_end(runway_end_id)
);

create index if not exists idx_ils_loc_runway_end_id on ils(loc_runway_end_id);
create index if not exists idx_ils_loc_airport_ident on ils(loc_airport_ident);
create index if not exists idx_ils_loc_runway_name on ils(loc_runway_name);

-- **************************************************

drop table if exists tmp_waypoint;

create table tmp_waypoint
(
  waypoint_id integer primary key,
  type varchar(15) not null,
  ident varchar(5) not null,
  region varchar(2) not null,
  lonx double not null,
  laty double not null
);

create index if not exists idx_tmp_waypoint_ident on tmp_waypoint(ident);
create index if not exists idx_tmp_waypoint_region on tmp_waypoint(region);
create index if not exists idx_tmp_waypoint_type on tmp_waypoint(type);

drop table if exists tmp_airway_point;

-- Airway segment as read from the FSX/P3D BGL record - this is a temp table and will be dropped later
-- This table is filled from temp_airway in case of X-Plane
create table tmp_airway_point
(
  airway_point_id integer primary key,
  waypoint_id integer,
  name varchar(50) not null,        -- Airway name
  type varchar(15),                 -- see enum atools::fs::bgl::nav::AirwayType
  mid_type varchar(15),             -- See enum atools::fs::bgl::nav::AirwayWaypointType
  mid_ident varchar(5),             -- ICAO ident of waypoint
  mid_region varchar(2),            -- ICAO two letter region code for waypoint
  next_direction varchar(1),        -- N = none, B = backward, F = forward
  next_type varchar(15),            -- See enum atools::fs::bgl::nav::AirwayWaypointType
  next_ident varchar(5),            -- ICAO ident of waypoint
  next_region varchar(2),           -- ICAO two letter region code for waypoint
  next_airport_ident,               -- Airport ICAO ident for airport if applicable
  next_minimum_altitude integer,    -- Minimum altitude for next segment in feet
  next_maximum_altitude integer,    -- Maximum altitude for next segment in feet
  previous_direction varchar(1),    -- Same as next
  previous_type varchar(15),        -- "
  previous_ident varchar(5),        -- "
  previous_region varchar(2),       -- "
  previous_airport_ident,           -- "
  previous_minimum_altitude integer,-- "
  previous_maximum_altitude integer -- "
);

create index if not exists idx_tmp_airway_point_waypoint_id on tmp_airway_point(waypoint_id);
create index if not exists idx_tmp_airway_point_waypoint_name on tmp_airway_point(name);
create index if not exists idx_tmp_airway_point_waypoint_mid_type on tmp_airway_point(mid_type);
create index if not exists idx_tmp_airway_point_waypoint_mid_ident on tmp_airway_point(mid_ident);
create index if not exists idx_tmp_airway_point_waypoint_mid_region on tmp_airway_point(mid_region);
create index if not exists idx_tmp_airway_point_waypoint_previous_type on tmp_airway_point(previous_type);
create index if not exists idx_tmp_airway_point_waypoint_previous_ident on tmp_airway_point(previous_ident);
create index if not exists idx_tmp_airway_point_waypoint_previous_region on tmp_airway_point(previous_region);
create index if not exists idx_tmp_airway_point_waypoint_next_type on tmp_airway_point(next_type);
create index if not exists idx_tmp_airway_point_waypoint_next_ident on tmp_airway_point(next_ident);
create index if not exists idx_tmp_airway_point_waypoint_next_region on tmp_airway_point(next_region);

-- **************************************************

drop table if exists airway_temp;

-- Airway segment as read from the X-Plane earth_awy.dat file- this is a temp table and will be dropped later
create table airway_temp
(
  airway_temp_id integer primary key,
  name varchar(50) not null,
  type integer,
  direction varchar(1),
  minimum_altitude integer,
  maximum_altitude integer,
  previous_type integer,
  previous_ident varchar(5),
  previous_region varchar(2),
  next_type integer,
  next_ident varchar(5),
  next_region varchar(2)
);

-- **************************************************

drop table if exists airway;

-- Processed and ordered airway segment that contains references to navaids instead of text identifiers
create table airway
(
  airway_id integer primary key,
  airway_name varchar(5) not null,     -- Airway name
  airway_type varchar(15) not null,    -- V = victor, J = jet, B = both
  route_type varchar(5),               -- ARINC 5.7 Enroute Airway Records (ER), Airway Type
                                       -- A Airline Airway (Tailored Data)
                                       -- C Control
                                       -- D Direct Route
                                       -- H Helicopter Airways
                                       -- O Officially Designated Airways, except RNAV, Helicopter Airways
                                       -- R RNAV Airways
                                       -- S Undesignated ATS Route
  airway_fragment_no integer not null, -- Designates a not connected fragment with the same name
  sequence_no integer not null,        -- Segment number
  from_waypoint_id integer not null,
  to_waypoint_id integer not null,
  direction varchar(1),                -- N = none, B = backward, F = forward
  minimum_altitude integer,
  maximum_altitude integer,
  left_lonx double not null,           -- Bounding rectangle of the segment
  top_laty double not null,            -- "
  right_lonx double not null,          -- "
  bottom_laty double not null,         -- "
  from_lonx double not null,           -- Segment start coordinates
  from_laty double not null,           -- "
  to_lonx double not null,             -- Segment end coordinates
  to_laty double not null,             -- "
foreign key(from_waypoint_id) references waypoint(waypoint_id),
foreign key(to_waypoint_id) references waypoint(waypoint_id)
);

create index if not exists idx_airway_from_waypoint_id on airway(from_waypoint_id);
create index if not exists idx_airway_to_waypoint_id on airway(to_waypoint_id);

-- **************************************************

drop table if exists nav_search;

-- Table that is used to search for mixed navaids. VOR, NDB and Waypoints.
-- Partially denormalized to speed up search.
create table nav_search
(
  nav_search_id integer primary key,
  waypoint_id integer,              -- waypoint ID if nav_type is waypoint
  waypoint_nav_id integer,          -- Id of a related VOR or NDB for the waypoint
  vor_id integer,                   -- VOR ID if nav_type is VORDME, VOR or DME
  ndb_id integer,                   -- NDB ID if tynav_typepe is NDB
  file_id integer not null,         -- BGL or dat file of the feature
  airport_id integer,               -- Related airport if applicable
  airport_ident varchar(4),         -- "
  ident varchar(5),                 -- ICAO ident for waypoint, VOR or NDB
  name varchar(50) collate nocase,  -- Name for case insensitive searching
  region varchar(2),                -- Two letter ICAO region code
  range integer,                    -- Range in NM if nav_type is VORDME, VOR, DME or NDB

  type varchar(15),     -- Subtype dependent on nav_type
                        -- 'VH'  - VOR/VORTAC - High
                        -- 'VL'  - VOR/VORTAC - Low
                        -- 'VT'  - VOR/VORTAC - Terminal
                        -- 'NHH' - NDB - HH
                        -- 'NH'  - NDB - H
                        -- 'NMH' - NDB - MH
                        -- 'NCP' - NDB - Compass Locator
                        -- 'WN'  - Waypoint - Named
                        -- 'WU'  - Waypoint - Unnamed
                        -- 'V'   - Waypoint - VOR
                        -- 'N'   - Waypoint - NDB

  nav_type varchar(15), -- WAYPOINT, VORDME, VOR, DME, NDB
                        -- (nav_type like ('V%') or nav_type in ('D', 'TC'))       All VOR/VORTAC/TACAN
                        -- (nav_type like ('V%') or nav_type in ('D', 'TC', 'N'))  All VOR/VORTAC/TACAN/NDB
                        -- nav_type in ('VD')                                      Only VOR-DME
                        -- nav_type in ('V')                                       Only VOR
                        -- nav_type in ('D')                                       Only DME
                        -- nav_type in ('VT')                                      Only VORTAC
                        -- nav_type in ('TC', 'TCD')                               Only TACAN
                        -- nav_type = 'N'                                          All NDB
                        -- nav_type = 'W'                                          All Waypoints

  arinc_type varchar(4),              -- ARINC Waypoint type as defined by the 3 columns of ARINC 424.18 field definition 5.42

  frequency integer,                  -- VOR: MHz * 10000, NDB kHz * 100 to allow differentiation between NDB and VOR
  channel varchar(10),                 --  TACAN channel of TACAN or VORTAC
  waypoint_num_victor_airway integer, -- Number of victor airways attached to this waypoint
  waypoint_num_jet_airway integer,    -- Number of jet airways attached to this waypoint
  scenery_local_path varchar(250) collate nocase,  -- Path relative to FS base directory
  bgl_filename varchar(300) collate nocase,        -- Filename
  mag_var double not null,            -- Magnetic variance in degree < 0 for West and > 0 for East
  altitude integer,                   -- Feet
  lonx double not null,
  laty double not null,
foreign key(waypoint_id) references waypoint(waypoint_id),
foreign key(vor_id) references vor(vor_id),
foreign key(ndb_id) references ndb(ndb_id),
foreign key(file_id) references bgl_file(bgl_file_id),
foreign key(airport_id) references airport(airport_id)
);

-- Used in FK but not in search
--create index if not exists idx_nav_search_waypoint_id on nav_search(waypoint_id);
--create index if not exists idx_nav_search_vor_id on nav_search(vor_id);
--create index if not exists idx_nav_search_ndb_id on nav_search(ndb_id);
--create index if not exists idx_nav_search_file_id on nav_search(file_id);
--create index if not exists idx_nav_search_airport_id on nav_search(airport_id);

-- **************************************************

drop table if exists magdecl;

-- Stores magnetic declination in binary format (see magdecreader.h)
-- Has only one row and is optional.
create table magdecl
(
  magdecl_id integer primary key,
  reference_time integer not null,  -- Reference time. Seconds since Epoch.
  mag_var blob                      -- A list of single precision float values prefixed by size
);

-- **************************************************

-- minimum off route altitude
drop table if exists mora_grid;

-- MORA grid
create table mora_grid
(
  mora_grid_id integer primary key,
  file_id integer not null,         -- BGL or dat file of the feature
  version integer not null,
  lonx_columns integer not null,
  laty_rows integer not null,
  geometry blob not null
);

-- **************************************************

drop table if exists airport_msa;

-- Airport minimum safe altitude
create table airport_msa
(
  airport_msa_id integer primary key,
  file_id integer not null,           -- BGL or dat file of the feature
  airport_id integer,                 -- Reference to airport
  airport_ident varchar(5) not null,  -- ICAO ident
  nav_id integer,                     -- Refers to airport.airport_id, vor.vor_id, ndb.ndb_id depending on type
  nav_ident varchar(5) not null,      -- ICAO ident
  region varchar(2),                  -- ICAO two letter region identifier
  multiple_code varchar(1),           -- ICAO ident
  type varchar(15),                   -- N = NDB, W = fix/waypoint, V = VOR/TACAN/DME, A = airport, R = runway end
  mag_var double,                     -- Magnetic variance in degree < 0 for West and > 0 for East
  left_lonx double,                   -- Bounding rectangle of the whole MSA sector
  top_laty double ,                   -- "
  right_lonx double,                  -- "
  bottom_laty double,                 -- "
  radius double not null,             -- Radius in NM
  lonx double not null,               -- Center coordinates
  laty double not null,
  geometry blob,                      -- Pre-calculated geometry as saved and loaded by
                                      -- atools::fs::common::BinaryMsaGeometry
foreign key(airport_id) references airport(airport_id)
);

-- **************************************************

drop table if exists holding;

-- Enroute holdings
create table holding
(
  holding_id integer primary key,
  file_id integer not null,           -- BGL or dat file of the feature
  airport_id integer,                 -- Reference to airport
  airport_ident varchar(5),           -- ICAO ident
  nav_id integer,                     -- Refers to vor.vor_id or ndb.ndb_id depending on type
  nav_ident varchar(5),               -- ICAO ident
  name varchar(50),
  region varchar(2),                  -- ICAO two letter region identifier
  type varchar(15),                   -- N = NDB, W = fix/waypoint, V = VOR/TACAN/DME, A = airport, R = runway end
  mag_var double,                     -- Magnetic variance in degree < 0 for West and > 0 for East
  course double not null,             -- True inbound course
  turn_direction varchar(1) not null, -- L or R
  leg_length double,                  -- Leg distance in NM
  leg_time double,                    -- Leg time in minutes
  minimum_altitude double,            -- Feet or null if not applicable
  maximum_altitude double,            -- Feet or null
  speed integer,                      -- Speed limit in knots or null
  lonx double not null,               -- Reference fix coordinates
  laty double not null,
foreign key(airport_id) references airport(airport_id)
);
