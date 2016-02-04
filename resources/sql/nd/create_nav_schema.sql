-- *****************************************************************************
-- Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

drop table if exists waypoint;

create table waypoint
(
  waypoint_id integer primary key,
  file_id integer not null,
  nav_id integer,
  ident text,
  region text,
  airport_id integer,
  type text not null,
  mag_var real not null,
  lonx real not null,
  laty real not null,
foreign key(file_id) references bgl_file(bgl_file_id),
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_waypoint_file_id on waypoint(file_id);
create index if not exists idx_waypoint_airport_id on waypoint(airport_id);
create index if not exists idx_waypoint_nav_id on waypoint(nav_id);

-- **************************************************

drop table if exists vor;

create table vor
(
  vor_id integer primary key,
  file_id integer not null,
  ident text not null,
  name text not null,
  region text,
  airport_id integer,
  type text not null,
  frequency integer not null,
  range integer not null,
  mag_var real  not null,
  dme_only integer not null,
  dme_altitude integer,
  dme_lonx real,
  dme_laty real,
  altitude integer not null,
  lonx real not null,
  laty real not null,
foreign key(file_id) references bgl_file(bgl_file_id),
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_vor_file_id on vor(file_id);
create index if not exists idx_vor_airport_id on vor(airport_id);

-- **************************************************

drop table if exists ndb;

create table ndb
(
  ndb_id integer primary key,
  file_id integer not null,
  ident text,
  name text not null,
  region text,
  airport_id integer,
  type text not null,
  frequency integer not null,
  range integer not null,
  mag_var real  not null,
  altitude integer not null,
  lonx real not null,
  laty real not null,
foreign key(file_id) references bgl_file(bgl_file_id),
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_ndb_file_id on ndb(file_id);
create index if not exists idx_ndb_airport_id on ndb(airport_id);

-- **************************************************

drop table if exists marker;

create table marker
(
  marker_id integer primary key,
  file_id integer not null,
  ident text,
  region text,
  type text not null,
  heading real not null,
  altitude integer not null,
  lonx real not null,
  laty real not null,
foreign key(file_id) references bgl_file(bgl_file_id)
);

create index if not exists idx_marker_file_id on marker(file_id);

-- **************************************************

drop table if exists ils;

create table ils
(
  ils_id integer primary key,
  ident text not null,
  name text,
  region text,
  frequency integer not null,
  range integer not null,
  mag_var real  not null,
  is_backcourse integer not null,
  dme_range integer,
  dme_altitude integer,
  dme_lonx real,
  dme_laty real,
  gs_range integer,
  gs_pitch real,
  gs_altitude integer,
  gs_lonx real,
  gs_laty real,
  loc_runway_end_id integer,
  loc_heading real not null,
  loc_width real not null,
  altitude integer not null,
  lonx real not null,
  laty real not null,
foreign key(loc_runway_end_id) references runway_end(runway_end_id)
);

create index if not exists idx_ils_loc_runway_end_id on ils(loc_runway_end_id);

-- **************************************************

drop table if exists temp_route;

create table temp_route
(
  temp_route_id integer primary key,
  waypoint_id integer not null,
  name text not null,
  type text not null,
  next_type text,
  next_ident text,
  next_region text,
  next_airport_ident,
  next_minimum_altitude integer,
  previous_type text,
  previous_ident text,
  previous_region text,
  previous_airport_ident,
  previous_minimum_altitude integer,
foreign key(waypoint_id) references waypoint(waypoint_id)
);

create index if not exists idx_temp_route_loc_waypoint_id on temp_route(waypoint_id);

-- **************************************************

drop table if exists route;

create table route
(
  route_id integer primary key,
  route_name text not null,
  route_type text not null,
  route_fragment_no integer not null,
  sequence_no integer not null,
  from_waypoint_id integer not null,
  to_waypoint_id integer not null,
  minimum_altitude integer,
foreign key(from_waypoint_id) references waypoint(waypoint_id),
foreign key(to_waypoint_id) references waypoint(waypoint_id)
);

create index if not exists idx_route_from_waypoint_id on route(from_waypoint_id);
create index if not exists idx_route_to_waypoint_id on route(to_waypoint_id);
