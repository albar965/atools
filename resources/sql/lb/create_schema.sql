-- *****************************************************************************
-- Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

-- Schema for the atools::fs::ln::LogbookLoader class

drop table if exists logbook;

-- Table holding logbook entries read from Logbook.BIN of FSX.
-- The table is denormalized to speed up searches and does not need
-- the optional airport table.
-- Some fields are defined for case insensitive searching
create table logbook
(
  logbook_id integer not null,                  -- Generated key based on the ordering in the binary files
  simulator_id integer not null,                -- Simulator 0=FSX, 1=FSXSE, 2=P3DV2 or 3=P3DV3
  startdate integer,                            -- Start date and time of the flight in seconds from epoch
  airport_from_icao text,                       -- Airport three of four letter code of start airport
  airport_from_name text collate nocase,        -- (*) Airport name extracted from airport table
  airport_from_city text collate nocase,        -- (*) City extracted from airport table
  airport_from_state text collate nocase,       -- (*) State or province from airport table
  airport_from_country text collate nocase,     -- (*) Country extracted from airport table
  airport_to_icao text,                         -- Airport three of four letter code of destination airport
  airport_to_name text collate nocase,          -- (*) Airport name extracted from airport table
  airport_to_city text collate nocase,          -- (*) City extracted from airport table
  airport_to_state text collate nocase,         -- (*) State or province from airport table
  airport_to_country text collate nocase,       -- (*) Country extracted from airport table
  distance real,                                -- (*) Shortest circle distance between start and destination
                                                --     airport in nautical miles
  description text collate nocase,              -- Description taken from Logbook.BIN
  total_time real not null,                     -- Total flight time in minutes
  night_time real not null,                     -- Night flight time in minutes
  instrument_time real not null,                -- IFR flight time in minutes
  aircraft_reg text collate nocase,             -- Aircraft registration
  aircraft_descr text  collate nocase not null, -- Aircraft description
  aircraft_type integer not null,               -- Type of aircraft - see view definition below
  aircraft_flags integer not null,              -- Misc flags for aircraft - see view definition below
  visits text,                                   -- Number of intermediate visits
primary key (logbook_id, simulator_id)
);
-- (*) only populated if airport table was found and not empty

drop table if exists logbook_visits;

-- Intermediate visits
create table logbook_visits
(
  visit_id integer PRIMARY KEY,  -- Generated ID
  logbook_id integer not null,   -- Points to logbook.logbook_id
  simulator_id integer not null, -- Points to logbook.simulator_id
  airport text,                  -- ICAO code
  landings integer,              -- Number of landings at this airport
foreign key(logbook_id, simulator_id) references logbook(logbook_id, simulator_id)
);


drop view if exists logbook_all;

-- View to show most of the logbook columns decoded for debugging purposes
create view logbook_all as
SELECT
  l.simulator_id,
  datetime(l.startdate, 'unixepoch') as startdate,
  l.airport_from_icao,
  l.airport_to_icao,
  l.description,
  printf('%d:%02d', cast(l.total_time as int), round((l.total_time - cast(l.total_time as int)) * 60)) as total_time,
  printf('%d:%02d', cast(l.night_time as int), round((l.night_time - cast(l.night_time as int)) * 60)) as night_time,
  printf('%d:%02d', cast(l.instrument_time as int), round((l.instrument_time - cast(l.instrument_time as int)) * 60)) as instrument_time,
  l.aircraft_reg as aircraft_registration,
  l.aircraft_descr as aircraft_description,
  CASE
    WHEN l.aircraft_type =0 THEN 'Unknown'
    WHEN l.aircraft_type =1 THEN 'Glider'
    WHEN l.aircraft_type =2 THEN 'Fixed Wing'
    WHEN l.aircraft_type =3 THEN 'Amphibious'
    WHEN l.aircraft_type =4 THEN 'Rotor'
    ELSE 'unknown'
  END as aircraft_type,
  case
    when l.aircraft_flags & 16384 = 16384 then 'Multi-engine'
    else NULL
  END as aircraft_flags
FROM logbook l
order by logbook_id desc;


drop view if exists logbook_all_visits;

-- View to show most of the logbook and visit columns decoded for debugging purposes
create view logbook_all_visits as
SELECT
  l.simulator_id,
  datetime(l.startdate, 'unixepoch') as startdate,
  l.airport_from_icao,
  l.airport_to_icao,
  l.description,
  printf("%d:%02d", cast(l.total_time as int), round((l.total_time - cast(l.total_time as int)) * 60)) as total_time,
  printf("%d:%02d", cast(l.night_time as int), round((l.night_time - cast(l.night_time as int)) * 60)) as night_time,
  printf("%d:%02d", cast(l.instrument_time as int), round((l.instrument_time - cast(l.instrument_time as int)) * 60)) as instrument_time,
  l.aircraft_reg as aircraft_registration,
  l.aircraft_descr as aircraft_description,
  CASE
    WHEN l.aircraft_type =0 THEN 'Unknown'
    WHEN l.aircraft_type =1 THEN 'Glider'
    WHEN l.aircraft_type =2 THEN 'Fixed Wing'
    WHEN l.aircraft_type =3 THEN 'Amphibious'
    WHEN l.aircraft_type =4 THEN 'Rotor'
  ELSE 'unknown'
  END as aircraft_type,
  case
    when l.aircraft_flags & 16384 = 16384 then 'Multi-engine'
    else NULL
  END as aircraft_flags,
  v.airport as visited_airport,
  v.landings
FROM logbook l left outer join logbook_visits v on l.logbook_id = v.logbook_id and l.simulator_id = v.simulator_id
order by l.logbook_id desc;

