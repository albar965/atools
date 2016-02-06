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

drop table if exists airport;

create table airport
(
  airport_id integer primary key,
  file_id integer not null,
  ident varchar(4) not null,
  region varchar(50),
  name varchar(50),
  country varchar(50) not null,
  state varchar(50),
  city varchar(50) not null,
  fuel_flags integer not null,
  has_avgas integer not null,
  has_jetfuel integer not null,
  has_tower_object integer not null,
  mag_var double not null,
  tower_lonx double,
  tower_laty double,
  altitude integer not null,
  lonx double not null,
  laty double not null,
foreign key(file_id) references bgl_file(bgl_file_id)
);

create index if not exists idx_airport_file_id on airport(file_id);
create index if not exists idx_airport_ident on airport(ident);

-- **************************************************

drop table if exists com;

create table com
(
  com_id integer primary key,
  airport_id not null,
  type varchar(30) not null,
  frequency integer not null,
  name varchar(50),
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_com_airport_id on com(airport_id);

-- **************************************************

drop table if exists helipad;

create table helipad
(
  helipad_id integer primary key,
  airport_id not null,
  surface varchar(15) not null,
  type varchar(10),
  length double not null,
  width double not null,
  heading double not null,
  is_transparent integer not null,
  is_closed integer not null,
  altitude integer not null,
  lonx double not null,
  laty double not null,
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_helipad_airport_id on helipad(helipad_id);

-- **************************************************

drop table if exists start;

create table start
(
  start_id integer primary key,
  airport_id not null,
  runway_end_id integer,
  type varchar(10) not null,
  heading double not null,
  altitude integer not null,
  lonx double not null,
  laty double not null,
foreign key(airport_id) references airport(airport_id),
foreign key(runway_end_id) references runway_end(runway_end_id)
);

create index if not exists idx_start_airport_id on start(airport_id);
create index if not exists idx_start_runway_end_id on start(runway_end_id);

-- **************************************************

drop table if exists apron;

create table apron
(
  apron_id integer primary key,
  airport_id not null,
  surface varchar(15) not null,
  is_draw_surface integer not null,
  is_draw_detail integer not null,
  vertices text not null,
  vertices2 text not null,
  triangles text not null,
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_apron_airport_id on apron(airport_id);

-- **************************************************

drop table if exists apron_light;

create table apron_light
(
  apron_light_id integer primary key,
  airport_id not null,
  vertices text not null,
  edges text not null,
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_apron_light_airport_id on apron_light(airport_id);

-- **************************************************

drop table if exists fence;

create table fence
(
  fence_id integer primary key,
  airport_id not null,
  type varchar(15) not null,
  vertices text not null,
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_fence_airport_id on fence(airport_id);

-- **************************************************

drop table if exists taxi_path;

create table taxi_path
(
  taxi_path_id integer primary key,
  airport_id not null,
  type varchar(15),
  surface varchar(15),
  width double not null,
  name varchar(20),
  is_draw_surface integer not null,
  is_draw_detail integer not null,
  has_centerline integer not null,
  has_centerline_light integer not null,
  has_left_edge_light integer not null,
  has_right_edge_light integer not null,
  start_type varchar(15),
  start_dir varchar(15),
  start_lonx double not null,
  start_laty double not null,
  end_type varchar(15),
  end_dir varchar(15),
  end_lonx double not null,
  end_laty double not null,
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_taxi_path_airport_id on taxi_path(airport_id);

-- **************************************************

drop table if exists runway;

create table runway
(
  runway_id integer primary key,
  airport_id not null,
  primary_end_id integer not null,
  secondary_end_id integer  not null,
  surface varchar(15) not null,
  length integer not null,
  width integer not null,
  heading double not null,
  pattern_altitude integer not null,
  marking_flags integer not null,
  light_flags integer not null,
  pattern_flags integer not null,
  edge_light varchar(15),
  center_light varchar(15),
  has_center_red integer not null,
  altitude integer not null,
  lonx double not null,
  laty double not null,
foreign key(airport_id) references airport(airport_id),
foreign key(primary_end_id) references runway_end(runway_end_id),
foreign key(secondary_end_id) references runway_end(runway_end_id)
);

create index if not exists idx_runway_airport_id on runway(airport_id);
create index if not exists idx_runway_primary_end_id on runway(primary_end_id);
create index if not exists idx_runway_secondary_end_id on runway(secondary_end_id);

-- **************************************************

drop table if exists runway_end;

create table runway_end
(
  runway_end_id integer primary key,
  name varchar(10) not null,
  offsetThreshold integer not null,
  blastPad integer not null,
  overrun integer not null,
  left_vasi_type varchar(15),
  left_vasi_pitch double,
  right_vasi_type varchar(15),
  right_vasi_pitch double,
  has_closed_markings integer not null,
  has_stol_markings integer not null,
  is_takeoff integer not null,
  is_landing integer not null,
  is_pattern varchar(10) not null,
  app_light_system_type varchar(15),
  has_end_lights integer not null,
  has_reils integer not null,
  has_touchdown_lights integer not null,
  num_strobes integer not null
);

create index if not exists idx_runway_end_name on runway_end(name);

-- **************************************************

drop table if exists approach;

create table approach
(
  approach_id integer primary key,
  runway_end_id integer,
  type varchar(25) not null,
  has_gps_overlay integer not null,
  fix_nav_id integer,
  fix_type varchar(25),
  fix_ident varchar(5),
  fix_region varchar(2),
  fix_airport_ident varchar(4),
  altitude integer,
  heading double,
  missed_altitude integer,
foreign key(runway_end_id) references runway_end(runway_end_id)
);

create index if not exists idx_approach_runway_end_id on approach(runway_end_id);

-- **************************************************

drop table if exists transition;

create table transition
(
  transition_id integer primary key,
  approach_id integer not null,
  type varchar(25) not null,
  fix_nav_id integer,
  fix_type varchar(25),
  fix_ident varchar(5),
  fix_region varchar(2),
  fix_airport_ident varchar(4),
  altitude integer,
  dme_ident varchar(5),
  dme_region varchar(2),
  dme_airport_ident,
  dme_radial double,
  dme_distance integer,
foreign key(approach_id) references approach(approach_id)
);

create index if not exists idx_transition_approach_id on transition(approach_id);

-- **************************************************

drop table if exists approach_leg;

create table approach_leg
(
  approach_leg_id integer primary key,
  approach_id integer not null,
  is_missed integer not null,

  type varchar(25) not null,
  alt_descriptor varchar(10),
  turn_direction varchar(10),
  fix_nav_id integer,
  fix_type varchar(25),
  fix_ident varchar(5),
  fix_region varchar(2),
  fix_airport_ident varchar(4),
  recommended_fix_nav_id integer,
  recommended_fix_type varchar(25),
  recommended_fix_ident varchar(5),
  recommended_fix_region varchar(2),
  is_flyover integer not null,
  is_true_course integer not null,
  course double,
  distance double,
  time double,
  theta double,
  rho double,
  altitude1 double,
  altitude2  double,
foreign key(approach_id) references approach(approach_id)
);

create index if not exists idx_approach_leg_approach_id on approach_leg(approach_id);


-- **************************************************

drop table if exists transition_leg;

create table transition_leg
(
  transition_leg_id integer primary key,
  transition_id integer not null,

  type varchar(25) not null,
  alt_descriptor varchar(10),
  turn_direction varchar(10),
  fix_nav_id integer,
  fix_type varchar(25),
  fix_ident varchar(5),
  fix_region varchar(2),
  fix_airport_ident varchar(4),
  recommended_fix_nav_id integer,
  recommended_fix_type varchar(25),
  recommended_fix_ident varchar(5),
  recommended_fix_region varchar(2),
  is_flyover integer not null,
  is_true_course integer not null,
  course double,
  distance double,
  time double,
  theta double,
  rho double,
  altitude1 double,
  altitude2  double,
foreign key(transition_id) references transition(transition_id)
);

create index if not exists idx_transition_leg_transition_id on transition_leg(transition_id);

-- **************************************************

drop table if exists parking;

create table parking
(
  parking_id integer primary key,
  airport_id integer not null,
  type varchar(20) not null,
  name varchar(15),
  number integer not null,
  radius double not null,
  heading double not null,
  has_jetway integer not null,
  lonx double not null,
  laty double not null,
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_parking_airport_id on parking(airport_id);

-- **************************************************

drop table if exists delete_airport;

create table delete_airport
(
  delete_airport_id integer primary key,
  airport_id integer not null,
  num_del_runway integer not null,
  num_del_start integer not null,
  num_del_com integer not null,
  approaches integer not null,
  apronlights integer not null,
  aprons  integer not null,
  frequencies  integer not null,
  helipads  integer not null,
  runways  integer not null,
  starts  integer not null,
  taxiways  integer not null,
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_delete_airport_airport_id on delete_airport(airport_id);
