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
  ident varchar(5),
  region varchar(2),
  airport_id integer,
  type varchar(15) not null,
  mag_var double not null,
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

create table vor
(
  vor_id integer primary key,
  file_id integer not null,
  ident varchar(3) not null,
  name varchar(50) not null,
  region varchar(2),
  airport_id integer,
  type varchar(15) not null,
  frequency integer not null,
  range integer not null,
  mag_var double  not null,
  dme_only integer not null,
  dme_altitude integer,
  dme_lonx double,
  dme_laty double,
  altitude integer not null,
  lonx double not null,
  laty double not null,
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
  ident varchar(3),
  name varchar(50) not null,
  region varchar(2),
  airport_id integer,
  type varchar(15) not null,
  frequency integer not null,
  range integer not null,
  mag_var double  not null,
  altitude integer not null,
  lonx double not null,
  laty double not null,
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
  ident varchar(3),
  region varchar(2),
  type varchar(15) not null,
  heading double not null,
  altitude integer not null,
  lonx double not null,
  laty double not null,
foreign key(file_id) references bgl_file(bgl_file_id)
);

create index if not exists idx_marker_file_id on marker(file_id);

-- **************************************************

drop table if exists ils;

create table ils
(
  ils_id integer primary key,
  ident varchar(4) not null,
  name varchar(50),
  region varchar(2),
  frequency integer not null,
  range integer not null,
  mag_var double  not null,
  has_backcourse integer not null,
  dme_range integer,
  dme_altitude integer,
  dme_lonx double,
  dme_laty double,
  gs_range integer,
  gs_pitch double,
  gs_altitude integer,
  gs_lonx double,
  gs_laty double,
  loc_runway_end_id integer,
  loc_heading double not null,
  loc_width double not null,
  end1_lonx double not null,
  end1_laty double not null,
  end_mid_lonx double not null,
  end_mid_laty double not null,
  end2_lonx double not null,
  end2_laty double not null,
  altitude integer not null,
  lonx double not null,
  laty double not null,
foreign key(loc_runway_end_id) references runway_end(runway_end_id)
);

create index if not exists idx_ils_loc_runway_end_id on ils(loc_runway_end_id);

-- **************************************************

drop table if exists route_point;

create table route_point
(
  route_point_id integer primary key,
  waypoint_id integer not null,
  name varchar(50) not null,
  type varchar(15) not null,
  next_type varchar(15),
  next_ident varchar(5),
  next_region varchar(2),
  next_airport_ident,
  next_minimum_altitude integer,
  previous_type varchar(15),
  previous_ident varchar(5),
  previous_region varchar(2),
  previous_airport_ident,
  previous_minimum_altitude integer,
foreign key(waypoint_id) references waypoint(waypoint_id)
);

create index if not exists idx_route_point_loc_waypoint_id on route_point(waypoint_id);

-- **************************************************

drop table if exists route;

create table route
(
  route_id integer primary key,
  route_name varchar(5) not null,
  route_type varchar(15) not null,
  route_fragment_no integer not null,
  sequence_no integer not null,
  from_waypoint_id integer not null,
  to_waypoint_id integer not null,
  minimum_altitude integer,
  left_lonx double not null,
  top_laty double not null,
  right_lonx double not null,
  bottom_laty double not null,
  from_lonx double not null,
  from_laty double not null,
  to_lonx double not null,
  to_laty double not null,
foreign key(from_waypoint_id) references waypoint(waypoint_id),
foreign key(to_waypoint_id) references waypoint(waypoint_id)
);

create index if not exists idx_route_from_waypoint_id on route(from_waypoint_id);
create index if not exists idx_route_to_waypoint_id on route(to_waypoint_id);

-- **************************************************

drop table if exists nav_search;

create table nav_search
(
  nav_search_id integer primary key,
  waypoint_id integer,
  waypoint_nav_id integer,
  vor_id integer,
  ndb_id integer,
  file_id integer not null,
  airport_id integer,
  airport_ident varchar(4),
  ident varchar(5),
  name varchar(50) collate nocase,
  region varchar(2),
  range integer,
  type varchar(15) not null, -- NAMED, UNNAMED -- HIGH, LOW, TERMINAL -- HH, H, MH, COMPASS_POINT
  nav_type varchar(15) not null, -- WAYPOINT, VORDME, VOR, DME, NDB
  frequency integer,
  scenery_local_path varchar(250) collate nocase not null,
  bgl_filename varchar(300) collate nocase not null,
  mag_var double not null,
  altitude integer,
  lonx double not null,
  laty double not null,
foreign key(waypoint_id) references waypoint(waypoint_id),
foreign key(vor_id) references vor(vor_id),
foreign key(ndb_id) references ndb(ndb_id),
foreign key(file_id) references bgl_file(bgl_file_id),
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_nav_search_waypoint_id on nav_search(waypoint_id);
create index if not exists idx_nav_search_vor_id on nav_search(vor_id);
create index if not exists idx_nav_search_ndb_id on nav_search(ndb_id);
create index if not exists idx_nav_search_file_id on nav_search(file_id);
create index if not exists idx_nav_search_airport_id on nav_search(airport_id);

