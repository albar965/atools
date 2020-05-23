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

-- *******************************************************************************
-- This script creates the logbook table which is kept in its own database
-- *******************************************************************************

drop table if exists logbook;

create table logbook
(
  logbook_id integer primary key,

  -- Aircraft ****************************************
  aircraft_name varchar(250) collate nocase,          -- Cessna 172 (model)
  aircraft_type varchar(250) collate nocase,          -- ICAO type descriptor like B732
  aircraft_registration varchar(50) collate nocase,   -- e.g. N12345

  -- Flightplan **************************************
  flightplan_number varchar(100) collate nocase,      -- Flight number if available
  flightplan_cruise_altitude double,                  -- Flight plan cruise altitude in feet

  -- Files ******************************************
  flightplan_file varchar(1024) collate nocase,       -- Full path to flight plan file
  performance_file varchar(1024) collate nocase,      -- Full path to performance file

  -- Trip *******************************************
  block_fuel double,                                  -- From aircraft performance - lbs
  trip_fuel double,                                   --  "
  used_fuel double,                                   --  "
  is_jetfuel integer,                                 -- Calculated from aircraft fuel - 1 = yes
  grossweight double,                                 -- Weight at takeoff - lbs
  distance double,                                    -- Flight plan distance in NM
  distance_flown double,                              -- Actual flown distance in NM

  -- Departure ***************************************
  departure_ident varchar(10) collate nocase,         -- Airport ICAO code
  departure_name varchar(200) collate nocase,         -- Airport name
  departure_runway varchar(10) collate nocase,        -- Runway if available
  departure_lonx double,                              -- Coordinates if available and airport resolves
  departure_laty double,                              -- "
  departure_alt double,                               -- Elevation in feet
  departure_time varchar(100),                        -- Real world departure time in local time
  departure_time_sim varchar(100),                    -- Simulator departure time in UTC

  -- Destination *************************************
  destination_ident varchar(10) collate nocase,       -- Same as above for destination
  destination_name varchar(200) collate nocase,       -- "
  destination_runway varchar(10) collate nocase,      -- "
  destination_lonx double,                            -- "
  destination_laty double,                            -- "
  destination_alt  double,                            -- "
  destination_time varchar(100),                      -- "
  destination_time_sim varchar(100),                  -- "

  route_string varchar(1024         ),               -- ICAO route string
  simulator varchar(50) collate nocase,              -- X-Plane 11, Prepar3D v4, etc.
  description varchar(2048) collate nocase,          -- Free text by user
  flightplan blob,                                   -- LNMPLN Gzipped XML file recorded on touchdown
  aircraft_perf blob,                                -- LNMPERF Gzipped XML file recorded on touchdown
  aircraft_trail blob                                -- Gzipped GPX aircraft trail file recorded on touchdown
  );

create index if not exists idx_logbook_aircraft_name on logbook(aircraft_name);
create index if not exists idx_logbook_aircraft_type on logbook(aircraft_type);
create index if not exists idx_logbook_aircraft_registration on logbook(aircraft_registration);
create index if not exists idx_logbook_flightplan_number on logbook(flightplan_number);
create index if not exists idx_logbook_flightplan_cruise_altitude on logbook(flightplan_cruise_altitude);
create index if not exists idx_logbook_distance on logbook(distance);
create index if not exists idx_logbook_distance_flown on logbook(distance_flown);
create index if not exists idx_logbook_departure_ident on logbook(departure_ident);
create index if not exists idx_logbook_departure_name on logbook(departure_name);
create index if not exists idx_logbook_departure_lonx on logbook(departure_lonx);
create index if not exists idx_logbook_departure_laty on logbook(departure_laty);
create index if not exists idx_logbook_departure_time on logbook(departure_time);
create index if not exists idx_logbook_departure_time_sim on logbook(departure_time_sim);
create index if not exists idx_logbook_destination_ident on logbook(destination_ident);
create index if not exists idx_logbook_destination_name on logbook(destination_name);
create index if not exists idx_logbook_destination_lonx on logbook(destination_lonx);
create index if not exists idx_logbook_destination_laty on logbook(destination_laty);
create index if not exists idx_logbook_destination_time on logbook(destination_time);
create index if not exists idx_logbook_destination_time_sim on logbook(destination_time_sim);
create index if not exists idx_logbook_simulator on logbook(simulator);
