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

-- **************************************************

drop table if exists trackmeta;

-- Metadata of tracks for display in tables
create table trackmeta
(
  trackmeta_id integer primary key,
  track_name varchar(15) not null,         -- Track name, letter or number
  track_type varchar(1) not null,          -- N = NATS track, P = PACOTS track, A = AUSOTS track
  route varchar(255) not null,             -- Track name, letter or number
  valid_from  varchar(50) not null,        -- Date time valid from UTC
  valid_to varchar(50) not null,           -- "
  download_timestamp varchar(50) not null  -- Date of download - local time
);

-- **************************************************

drop table if exists track;

-- Processed and ordered track segment that contains references to navaids instead of text identifiers
-- Some columns are duplicated with trackmeta for simpler queries
-- From/to is always in track order/direction.
create table track
(
  track_id integer primary key,        -- Id uses offset to have it unique with airways
  trackmeta_id integer not null,
  track_name varchar(15) not null,     -- Track name, letter or number
  track_type varchar(1) not null,      -- N = NATS track, P = PACOTS track, A = AUSOTS track
  track_fragment_no integer not null,  -- Designates a not connected fragment with the same name - needed for queries
  sequence_no integer not null,        -- Segment number

  -- Data inherited from underlying airway if any
  airway_id integer,                   -- Airway information from underlying airway if any
  airway_minimum_altitude integer,     -- From airway if any
  airway_maximum_altitude integer,     -- "
  airway_direction varchar(1),         -- N = none, B = backward, F = forward

  from_waypoint_id integer,            -- True or generated waypoint - generated use an id offset to be unique with true
  from_waypoint_name varchar(15),      -- Original name - also for coordinate formats
  to_waypoint_id integer,              -- "
  to_waypoint_name varchar(15),        -- "
  altitude_levels_east blob,           -- NATS levels - first unsigned short is number of levels with levels following as unsigned short
  altitude_levels_west blob,           -- "
  left_lonx double not null,           -- Bounding rectangle of the segment
  top_laty double not null,            -- "
  right_lonx double not null,          -- "
  bottom_laty double not null,         -- "
  from_lonx double not null,           -- Segment start coordinates
  from_laty double not null,           -- "
  to_lonx double not null,             -- Segment end coordinates
  to_laty double not null,
foreign key(trackmeta_id) references trackmeta(trackmeta_id),
foreign key(from_waypoint_id) references trackpoint(trackpoint_id),
foreign key(to_waypoint_id) references trackpoint(trackpoint_id)
);

create index if not exists idx_track_type on track(track_type);
create index if not exists idx_track_from_waypoint_id on track(from_waypoint_id);
create index if not exists idx_track_to_waypoint_id on track(to_waypoint_id);

create index if not exists idx_track_left_lonx on track(left_lonx);
create index if not exists idx_track_top_laty on track(top_laty);
create index if not exists idx_track_right_lonx on track(right_lonx);
create index if not exists idx_track_bottom_laty on track(bottom_laty);

create index if not exists idx_track_from_lonx on track(from_lonx);
create index if not exists idx_track_from_laty on track(from_laty);
create index if not exists idx_track_to_lonx on track(to_lonx);
create index if not exists idx_track_to_laty on track(to_laty);

-- **************************************************

drop table if exists trackpoint;

-- Waypoint/intersection navaid which is part of a track
-- Contains either copies of actual waypoints or generated waypoints from track coordinates.
-- The latter one use an id with offset
create table trackpoint
(
  trackpoint_id integer primary key,  -- Id uses offset to have it unique with waypoints if generated
                                      -- Otherweise waypoint_id
  nav_id integer,                     -- Refers to vor.vor_id or ndb.ndb_id depending on type
  ident varchar(5),                   -- ICAO ident
  region varchar(2),                  -- ICAO two letter region identifier
  type varchar(15),                   -- see enum atools::fs::bgl::nav::WaypointType
                                      -- N = NDB, OA = off airway, V = VOR, WN = named waypoint,
                                      -- WU = unnamed waypoint, WT = artificial track waypoint
  num_victor_airway integer not null, -- Number of victor (low altitude) airways crossing this waypoint
  num_jet_airway integer not null,    -- Number of jet (high altitude) airways crossing this waypoint
  mag_var double not null,            -- Magnetic variance in degree < 0 for West and > 0 for East
  lonx double not null,
  laty double not null
);

create index if not exists idx_trackpoint_nav_id on trackpoint(nav_id);
create index if not exists idx_trackpoint_ident on trackpoint(ident);
create index if not exists idx_trackpoint_region on trackpoint(region);
create index if not exists idx_trackpoint_lonx on trackpoint(lonx);
create index if not exists idx_trackpoint_laty on trackpoint(laty);
