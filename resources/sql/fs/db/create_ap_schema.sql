-- *****************************************************************************
-- Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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
-- This script create all airport related tables
-- *************************************************************


drop table if exists airport;

-- Airports. Can contain one ICAO ident multiple times depending on loading settings.
-- Some fields are redundant/denormalized to ease airport search
create table airport
(
  airport_id integer primary key,
  file_id integer not null,                   -- BGL or apt.dat file id
  ident varchar(10) not null,                 -- ICAO ident or X-Plane internal airport ID for duplicate ICAO
  icao varchar(10),                           -- ICAO ident if available and different from ident
  iata varchar(10),                           -- IATA ident if available
  faa varchar(10),                            -- FAA code if available
  local varchar(10),                          -- Local code if available
  name varchar(50) collate nocase,
  city varchar(50) collate nocase,
  state varchar(50) collate nocase,
  country varchar(50) collate nocase,         -- Country name or area code in case of DFD
  region varchar(4) collate nocase,           -- ICAO region like DE, LF, K6 - not used for search
  flatten integer,                            -- 1302 flatten 1. 0, 1 or null. X-Plane only.
  type integer,                               -- 1 Land airport, 16 Seaplane base, 17 Heliport. X-Plane only.
  fuel_flags integer not null,                -- see enum atools::fs::bgl::ap::FuelFlags
  has_avgas integer not null,                 -- boolean
  has_jetfuel integer not null,               -- "
  has_tower_object integer not null,          -- "

  tower_frequency integer,                    -- MHz * 1000 or MHz * 100000 for 8.33 kHz spacing
  atis_frequency integer,                     -- "
  awos_frequency integer,                     -- "
  asos_frequency integer,                     -- "
  unicom_frequency integer,                   -- "

  is_closed integer not null,                 -- boolean - 1 if all runway ends have closed markings
  is_military integer not null,               -- boolean - Military airports are recognized by name - see airport.cpp
  is_addon integer not null,                  -- boolean - 1 if BGL is not int the default scenery directory

  num_com integer not null,                   -- Number of COM frequencies

  num_parking_gate integer not null,
  num_parking_ga_ramp integer not null,
  num_parking_cargo  integer not null,
  num_parking_mil_cargo integer not null,
  num_parking_mil_combat integer not null,

  num_approach integer not null,
  num_runway_hard integer not null,
  num_runway_soft integer not null,
  num_runway_water integer not null,
  num_runway_light integer not null,         -- Number of runways that have edge lightings
  num_runway_end_closed integer not null,    -- Number of runway ends that have closed markings
  num_runway_end_vasi integer not null,      -- Number of runway ends that have a visual approach indicator (PAPI, etc.)
  num_runway_end_als integer not null,       -- Number of runway ends that have an approach lighting system
  num_runway_end_ils integer,                -- Number of runway ends that have an instrument landing system
                                             -- Nullable here since it will be updated later
  num_apron integer not null,
  num_taxi_path integer not null,
  num_helipad integer not null,
  num_jetway integer not null,               -- Number of parking spots that have jetways attached
  num_starts integer not null,               -- Number of start positions

  longest_runway_length integer not null,    -- Feet
  longest_runway_width integer not null,     -- Feet
  longest_runway_heading double not null,    -- Heading of primary end in degrees true
  longest_runway_surface varchar(15),        -- see enum atools::fs::bgl::Surface
                                             -- Additional surface types are unspecified hard "UH" and unspecified soft "US"
                                             -- which are used for data not originating from flight simulator
  num_runways integer not null,
  largest_parking_ramp varchar(20),          -- see enum atools::fs::bgl::ap::ParkingType
  largest_parking_gate varchar(20),          -- see enum atools::fs::bgl::ap::ParkingType

  rating integer not null,                   -- 0-5. An airport gets a point for having
                                             -- taxi paths, parking, aprons, tower object.
                                             -- An additional point is give for add-on airports
  is_3d integer not null,                    -- X-Plane only - airport has 3D objects

  scenery_local_path varchar(250) collate nocase, -- Path of the BGL relative to the FS base directory
  bgl_filename varchar(300) collate nocase,       -- BGL filename

  left_lonx double not null,                    -- Bounding rectangle of the airport
  top_laty double not null,                     -- "
  right_lonx double not null,                   -- "
  bottom_laty double not null,                  -- "

  mag_var double not null,                      -- Magnetic variance in degree < 0 for West and > 0 for East
  tower_altitude integer,                       -- Feet
  tower_lonx double,
  tower_laty double,

  transition_altitude double,                   -- Feet. Transition Altitude is the altitude when flying where you are required to change from a
                                                -- local QNH to the standard of 1013 hectopascals or 29.92 inches of mercury
  transition_level double,                      -- Feet. Transition Level is the altitude when flying where you are required to change
                                                -- from standard of 1013 back to the local QNH. This is above the Transition Altitude.

  altitude integer not null,                    -- Feet
  lonx double not null,                         -- Coordinates of the airport center
  laty double not null,                         -- Coordinates of the airport center
foreign key(file_id) references bgl_file(bgl_file_id)
);

-- **************************************************

drop table if exists airport_file;

-- Connects airport idents to filenames - this will remain intact event if airports are
-- removed by delete records
create table airport_file
(
  airport_file_id integer primary key,
  file_id integer not null,                   -- BGL file id
  ident varchar(4) not null,                  -- ICAO ident
foreign key(file_id) references bgl_file(bgl_file_id)
);

-- **************************************************

drop table if exists com;

-- All communication frequencies for an airport
create table com
(
  com_id integer primary key,
  airport_id integer not null,   -- Airport
  type varchar(30),              -- see enum atools::fs::bgl::ComType
  frequency integer not null,    -- frequency MHz * 1000
  name varchar(50),
foreign key(airport_id) references airport(airport_id)
);

-- **************************************************

drop table if exists helipad;

-- Helipads of an airport
create table helipad
(
  helipad_id integer primary key,
  airport_id integer not null,
  start_id integer,
  surface varchar(15),                 -- see enum atools::fs::bgl::Surface
  type varchar(10),                    -- see enum atools::fs::bgl::helipad::HelipadType
  length double not null,              -- Feet
  width double not null,               -- Feet
  heading double not null,             -- Degrees true
  is_transparent integer not null,     -- 1 if surface should not be drawn e.g. for photo scenery
  is_closed integer not null,          -- 1 if closed
  altitude integer not null,           -- Feet
  lonx double not null,
  laty double not null,
foreign key(airport_id) references airport(airport_id),
foreign key(start_id) references start(start_id)
);

-- **************************************************

drop table if exists start;

-- Airport start position that will show up in the FS "go to airport" dialog
create table start
(
  start_id integer primary key,
  airport_id integer not null,
  runway_end_id integer,
  runway_name varchar(10),      -- Runway name like "1L" or "NE". Needed if no runway is present.
  type varchar(10),             -- see enum atools::fs::bgl::start::StartType
  heading double not null,      -- Degrees true
  number integer,               -- Number used for helipads
  altitude integer not null,    -- Feet
  lonx double not null,
  laty double not null,
foreign key(airport_id) references airport(airport_id),
foreign key(runway_end_id) references runway_end(runway_end_id)
);

-- **************************************************

drop table if exists apron;

-- Airport apron
-- Contains pavement in case of X-Plane which includes taxiways
create table apron
(
  apron_id integer primary key,
  airport_id integer not null,
  surface varchar(15),              -- see enum atools::fs::bgl::Surface
  is_draw_surface integer not null, -- 0 if surface should not be drawn e.g. for photo scenery
  is_draw_detail integer not null,  -- Draw FS detail texture
  vertices blob,                    -- Coordinate list of the apron. Single precision float binary format.
                                    -- boundary (lon1 lat1, lon2 lat2, ...).
  vertices2 blob,                   -- Apron triangle vertices - not used
  triangles blob,                   -- Apron triangle vertex references - space and comma separated - not used
                                    -- vertex index for vertices2 (i1 i2 i3, i2 i3 i4, ...)
  geometry blob,                    -- Optional field: X-Plane apron and taxiway geometry
foreign key(airport_id) references airport(airport_id)
);

-- **************************************************

drop table if exists taxi_path;

-- A taxiway segment. Note that segments of the same name are not aggregated. Includes runway
-- and vehicle paths
create table taxi_path
(
  taxi_path_id integer primary key,
  airport_id integer not null,
  type varchar(15),                    -- see enum atools::fs::bgl::taxipath::Type
  surface varchar(15),                 -- see enum atools::fs::bgl::Surface
  width double not null,               -- Feet
  name varchar(20),                    -- Taxiway name like A, B, EAST, etc.
  is_draw_surface integer not null,    -- 0 if surface should not be drawn e.g. for photo scenery
  is_draw_detail integer not null,     -- Draw FS detail texture
  start_type varchar(15),                   -- Type of start point - see enum atools::fs::bgl::taxipoint::PointType
  start_dir varchar(15),                    -- Direction of start point - see enum atools::fs::bgl::taxipoint::PointDir
  start_lonx double not null,
  start_laty double not null,
  end_type varchar(15),                     -- Same as for start
  end_dir varchar(15),                      -- "
  end_lonx double not null,                 -- "
  end_laty double not null,                 -- "
foreign key(airport_id) references airport(airport_id)
);

-- **************************************************

drop table if exists runway;

-- Airport runway
create table runway
(
  runway_id integer primary key,
  airport_id integer not null,
  primary_end_id integer not null,      -- Reference to the secondary end
  secondary_end_id integer  not null,   -- Reference to the secondary end
  surface varchar(15),                  -- see enum atools::fs::bgl::Surface
                                        -- Additional surface types are unspecified hard "UH" and unspecified soft "US"
                                        -- which are used for data not originating from flight simulator
  smoothness double,                    -- 0.00 (smooth) to 1.00 (very rough). Default is 0.25. X-Plane only.
  shoulder varchar(15),                 -- Optional column for X-Plane - shoulder surface or null if none
  length double not null,              -- Feet
  width double not null,               -- Feet
  heading double not null,              -- Heading in degrees true
  pattern_altitude integer not null,    -- Feet
  marking_flags integer not null,       -- see enum atools::fs::bgl::rw::RunwayMarkings
  edge_light varchar(15),               -- see enum atools::fs::bgl::rw::Light
  center_light varchar(15),             -- see enum atools::fs::bgl::rw::Light
  has_center_red integer not null,      -- Boolean
  primary_lonx double not null,        -- Coordinates of the primary end
  primary_laty double not null,        -- Coordinates of the primary end
  secondary_lonx double not null,      -- Coordinates of the secondary end
  secondary_laty double not null,      -- Coordinates of the secondary end
  altitude integer not null,            -- Feet - center or average elevation
  lonx double not null,
  laty double not null,
foreign key(airport_id) references airport(airport_id),
foreign key(primary_end_id) references runway_end(runway_end_id),
foreign key(secondary_end_id) references runway_end(runway_end_id)
);

-- **************************************************

drop table if exists runway_end;

-- Runway ends for a runway
create table runway_end
(
  runway_end_id integer primary key,
  name varchar(10) not null,              -- Full name like "12", "24C" or "N"
  end_type varchar(1) not null,           -- Primary or secondary
  offset_threshold double not null,      -- Feet - this is part of the runway length and will reduce landing distance
  blast_pad double not null,             -- Feet - not part of the runway length
  overrun double not null,               -- Feet - not part of the runway length
  left_vasi_type varchar(15),             -- see enum atools::fs::bgl::rw::VasiType
  left_vasi_pitch double,                 -- Degrees
  right_vasi_type varchar(15),            -- see enum atools::fs::bgl::rw::VasiType
  right_vasi_pitch double,                -- Degrees
  has_closed_markings integer not null,   -- Threshow crossed out and runway closed for landing
  has_stol_markings integer not null,     -- Has STOL text
  is_takeoff integer not null,            -- Is used for takeoff or not
  is_landing integer not null,            -- Is used for landing or not
  is_pattern varchar(10) not null,        -- LEFT = 0, RIGHT = 1, "N" = not available
  app_light_system_type varchar(15),      -- see enum atools::fs::bgl::rw::ApproachLightSystem
  has_end_lights integer not null,        -- Boolean
  has_reils integer not null,             -- Boolean has runway end identifier lights or not
  has_touchdown_lights integer not null,  -- Boolean lighting for the touchdown zone
  num_strobes integer,                    -- Number of strobe lights - Obsolete, not used by Little Navmap
  ils_ident varchar(10),                  -- Ident of the ILS or null if none - only used during compilation
  heading double not null,                -- Duplicated from runway
  altitude integer,                       -- Threshold elevation for this end if available
  lonx double not null,                   -- "
  laty double not null                    -- "
);

-- **************************************************

drop table if exists approach;

-- Procedure for an airport. Can be a SID, STAR or an approach. Not necessarily assigned to a runway end.
-- SID and STAR are saved similar to FSX conventions:
-- STAR: suffix = 'A' and has_gps_overlay = 1
-- SID: suffix = 'D' and has_gps_overlay = 1
create table approach
(
  approach_id integer primary key,
  airport_id integer,
  runway_end_id integer,            -- Runway end id - can be null if the approach allows circling
  arinc_name varchar(6),            -- ARINC 424.18+, section 5.10 (APPRAOCH IDENT).
                                    -- Examples: I26L, B08R, R29 for approaches or
                                    -- runway identifier 5.11 (TRANS IDENT) for SID and STAR or "ALL"
  airport_ident varchar(4),         -- This is the original X-Plane CIFP airport ident which might not match the
                                    -- ident for the given airport by airport_id. May be ICAO, FAA or local code.
  runway_name varchar(10),          -- Runway name if procedure can be assigned to one single runway
  type varchar(25) not null,        -- see enum atools::fs::bgl::ap::ApproachType
  suffix varchar(1),                -- Approach suffix - indicates SID/STAR for P3D
                                    -- Both SIDS and STARS use the type = "GPS" for the Approach elements.
                                    -- STARS use the suffix="A" while SIDS use the suffix="D".
  has_gps_overlay integer not null, -- Boolean - 1 if the approach has a GPS overlay
  has_vertical_angle integer,       -- Boolean - 1 if the approach has a vertical path on any leg, 0 or null otherwise
  has_rnp integer,                  -- Boolean - 1 if the approach has a RNP value on any leg, 0 or null otherwise

  fix_type varchar(25),             -- see enum atools::fs::bgl::ap::ApproachFixType and corresponding string conversion
  fix_ident varchar(5),             -- ICAO ident of the approach fix or name of SID/STAR
  fix_region varchar(2),            -- ICAO two letter region code for fix
  fix_airport_ident varchar(4),     -- Airport ICAO ident if available

  aircraft_category varchar(4),     -- Aircraft category
                                    -- A All Aircraft
                                    -- C All Aircraft, Cruise speed 250 kts or less
                                    -- D Non-Jet and Turbo Prop
                                    -- E Multi-Engine Props Only
                                    -- F Jets and Turbo Props/Special, Cruise Speed 190 kts or greater
                                    -- H Helicopter Only
                                    -- J Jet Power
                                    -- M Turbo-Prop/Special, Cruise Speed 190 kts or greater
                                    -- N Non-Jet, Non-Turbo Prop
                                    -- P Non-Jet, Cruise speed 190 kts or greater
                                    -- Q Non-Jet, Cruise speed 189 kts or less
                                    -- R Aircraft as defined in a Notes Continuation Record
                                    -- S Single Engine
                                    -- T Twin Engine

  altitude integer,                 -- Crossing altitude for the fix in feet
  heading double,                   -- Heading in degrees true
  missed_altitude integer,          -- Missed approach altitude in feet
foreign key(airport_id) references airport(airport_id),
foreign key(runway_end_id) references runway_end(runway_end_id)
);

-- **************************************************

drop table if exists transition;

-- A transition to an approach procedure
create table transition
(
  transition_id integer primary key,
  approach_id integer not null,
  type varchar(25) not null,    -- see enum atools::fs::bgl::ap::TransitionType
  fix_type varchar(25),         -- "
  fix_ident varchar(5),         -- "
  fix_region varchar(2),        -- "
  fix_airport_ident varchar(4), -- "

  aircraft_category varchar(4), -- Aircraft category
                                -- A All Aircraft, Cruise speed 250 kts or less
                                -- C Non-Jet and Turbo Prop
                                -- D Multi-Engine Props Only
                                -- E Jets and Turbo Props/Special, Cruise Speed 190 kts
                                -- F or greater
                                -- H Helicopter Only
                                -- J Jet Power
                                -- M Turbo-Prop/Special, Cruise Speed 190 kts or greater
                                -- N Non-Jet, Non-Turbo Prop
                                -- P Non-Jet, Cruise speed 190 kts or greater
                                -- Q Non-Jet, Cruise speed 189 kts or less
                                -- R Aircraft as defined in a Notes Continuation Record
                                -- S Single Engine
                                -- T Twin Engine

  altitude integer,             -- Overfly altitude in feet for the transition fix
  dme_ident varchar(5),         -- Contains the DME ICAO ident if transition type is DME
  dme_region varchar(2),        -- ICAO two letter region code for DME if transition type is DME
  dme_airport_ident varchar(5),
  dme_radial double,            -- Radial point where the transition starts
  dme_distance integer,         -- DME distance in NM
foreign key(approach_id) references approach(approach_id)
);

-- **************************************************

drop table if exists approach_leg;

create table approach_leg
(
  approach_leg_id integer primary key,
  approach_id integer not null,
  is_missed integer not null,         -- 1 if this leg is part of a missed approach
  type varchar(10),                   -- see enum atools::fs::bgl::leg::Type
  arinc_descr_code varchar(25), -- ARINC description code 5.17
  approach_fix_type varchar(1),       -- A = IAF, M = MAP,  H = Holding fix, etc. see ARINC-424 spec
  alt_descriptor varchar(10),         -- see enum atools::fs::bgl::leg::AltDescriptor
                                      -- "A": at, "+": at or above, "-": at or below, "B": between altitude2 and altitude1
  turn_direction varchar(10),         -- see enum atools::fs::bgl::leg::TurnDirection
  rnp double,                         -- Required Navigation Performance - ARINC 5.211 - Examples:
                                      -- 990 (equal to 99.0NM), -- 120 (equal to 12.0NM), 013 --(equal to 0.001NM)
  fix_type varchar(25),               -- same as in approach
  fix_ident varchar(5),               -- "
  fix_region varchar(2),              -- "
  fix_airport_ident varchar(4),       -- "
  fix_lonx double,                    -- Optional coordinates to better find the correct navaid
  fix_laty double,                    -- "
  recommended_fix_type varchar(25),   -- " TODO needs seperate center navaid for RF legs
  recommended_fix_ident varchar(5),   -- "
  recommended_fix_region varchar(2),  -- "
  recommended_fix_lonx double,        -- "
  recommended_fix_laty double,        -- "
  is_flyover integer not null,        -- 1 if this is a flyover fix
  is_true_course integer not null,    -- 1 if course is degrees true
  course double,                      -- leg course in degrees true of magnetic depending on is_true_course
  distance double,                    -- Leg distance in NM - if null use time
  time double,                        -- Leg time - if null use distance
  theta double,                       -- Heading in degrees true
  rho double,                         -- Distance in NM
  altitude1 double,                   -- Altitude value 1 in feet. Meaning depends on alt_descriptor
  altitude2  double,                  -- Altitude value 2 in feet. Meaning depends on alt_descriptor
  speed_limit_type varchar(2),        -- null: mandatory speed if limit is given, "+": minimum speed, "-": maximum speed
  speed_limit integer,                -- null or speed limit in knots
  vertical_angle double,              -- Vertical angle in degree
foreign key(approach_id) references approach(approach_id)
);

-- **************************************************

drop table if exists transition_leg;

-- Transition leg. Columns are the same as for the approach leg
create table transition_leg
(
  transition_leg_id integer primary key,
  transition_id integer not null,
  type varchar(10) not null,
  arinc_descr_code varchar(25), -- ARINC description code 5.17
  approach_fix_type varchar(1),
  alt_descriptor varchar(10),
  turn_direction varchar(10),
  rnp double,                         -- Required Navigation Performance - ARINC 5.211
  fix_type varchar(25),
  fix_ident varchar(5),
  fix_region varchar(2),
  fix_airport_ident varchar(4),
  fix_lonx double,                    -- Optional coordinates to better find the correct navaid
  fix_laty double,                    -- "
  recommended_fix_type varchar(25),
  recommended_fix_ident varchar(5),
  recommended_fix_region varchar(2),
  recommended_fix_lonx double,        -- "
  recommended_fix_laty double,        -- "
  is_flyover integer not null,
  is_true_course integer not null,
  course double,
  distance double,
  time double,
  theta double,
  rho double,
  altitude1 double,
  altitude2  double,
  speed_limit_type varchar(2),  -- null: mandatory speed if limit is given, "+": minimum speed, "-": maximum speed
  speed_limit integer,          -- null or speed limit in knots
  vertical_angle double,        -- Vertical angle in degree
foreign key(transition_id) references transition(transition_id)
);

-- **************************************************

drop table if exists parking;

-- Parking spot. Includes fuel and vehicle parking.
create table parking
(
  parking_id integer primary key,
  airport_id integer not null,
  type varchar(20),                -- see enum atools::fs::bgl::ap::ParkingType
  pushback varchar(5),             -- see enum atools::fs::bgl::ap::PushBack
  name varchar(15),                -- see enum atools::fs::bgl::ap::ParkingName
  number integer not null,         -- parking number
  suffix varchar(5),               -- see enum atools::fs::bgl::ap::ParkingNameSuffix - only MSFS
  airline_codes text,              -- Comma separated list of two letter ICAO airline codes
  radius double,                   -- Radius in feet
  heading double,                  -- Heading in degree true
  has_jetway integer not null,     -- 1 if the parking has a jetway attached
  lonx double not null,
  laty double not null,
foreign key(airport_id) references airport(airport_id)
);


